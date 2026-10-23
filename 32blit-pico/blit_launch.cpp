#include <algorithm>
#include <cstring>
#include <forward_list>

#include "pico/stdlib.h"
#include "hardware/flash.h"
#include "hardware/sync.h"
#include "pico/multicore.h"

#ifdef PICO_RP2350
#include "hardware/structs/qmi.h"
#include "hardware/xip_cache.h"
#endif

#include "config.h"
#include "blit_launch.hpp"
#include "file.hpp"
#include "overlay.hpp"

#include "engine/engine.hpp"
#include "engine/file.hpp"

#ifdef PICO_RP2350
#define DEVICE_ID BlitDevice::RP2350
#define FLASH_BASE XIP_NOCACHE_NOALLOC_NOTRANSLATE_BASE
#else
#define DEVICE_ID BlitDevice::RP2040
#define FLASH_BASE XIP_NOCACHE_NOALLOC_BASE
#endif

// code related to blit files and launching

#ifdef STORAGE_SD
static const uint32_t flash_end = PICO_FLASH_SIZE_BYTES;
#else
static const uint32_t flash_end = FLASH_STORAGE_OFFSET;
#endif

struct TypeHandlerInfo {
  char type[4]; // should have a null terminator, but the low byte of the offset will always be zero
  uint32_t offset;
};

std::forward_list<TypeHandlerInfo> handlers;

#ifdef BUILD_LOADER
static char launch_path[256];
#endif

extern int (*do_tick)(uint32_t time);

void disable_user_code();

extern bool core1_started;

static uint32_t requested_launch_offset = 0;
static uint32_t current_game_offset = 0;

static uint32_t get_installed_file_size(uint32_t offset) {
  auto header = (BlitGameHeader *)(FLASH_BASE + offset);

  // check header magic + device
  if(header->magic != blit_game_magic || header->device_id != DEVICE_ID)
    return 0;

  auto size = header->end;

  // check metadata
  auto meta_offset = offset + size;
  if(memcmp((char *)(FLASH_BASE + meta_offset), "BLITMETA", 8) == 0) {
    // add metadata size
    size += *(uint16_t *)(FLASH_BASE + meta_offset + 8) + 10;
  }

  return size;
}

static uint32_t calc_num_blocks(uint32_t size) {
  return (size - 1) / game_block_size + 1;
}

#ifdef BUILD_LOADER

#ifdef PICO_RP2350
static uint32_t find_flash_offset(uint32_t requested_size) {
  uint32_t free_off = 0; // 0 is invalid as that's where the loader is

  for(uint32_t off = 256 * 1024; off < flash_end;) {
    auto size = get_installed_file_size(off);

    if(!size) {
      // empty, store offset
      if(!free_off)
        free_off = off;

      off += game_block_size;
      continue;
    }

    if(free_off) {
      // end of free space, check if large enough
      auto found_space = off - free_off;

      if(found_space >= requested_size)
        return free_off;

      free_off = 0;
    }

    // skip to end
    off += calc_num_blocks(size) * game_block_size;
  }

  // last chance
  if(free_off && flash_end - free_off >= requested_size)
    return free_off;

  return 0;
}
#endif

static bool read_file_metadata(void *file, RawMetadata &meta, RawTypeMetadata &type_meta) {
  // read header and check magic
  BlitGameHeader header;
  if(read_file(file, 0, sizeof(header), (char *)&header) != sizeof(header))
    return false;

  if(header.magic != blit_game_magic)
    return false;

  // read and check metadata header
  auto meta_offset = header.end;

  char meta_header[10];
  if(read_file(file, meta_offset, 10, meta_header) != 10)
    return false;

  if(memcmp(meta_header, "BLITMETA", 8) != 0)
    return false;

  // read the reset of the metadata header
  if(read_file(file, meta_offset + 10, sizeof(RawMetadata), (char *)&meta) != sizeof(RawMetadata))
    return false;

  // check for type data
  meta_offset += 10 + sizeof(RawMetadata);
  if(read_file(file, meta_offset, 8, meta_header) != 8)
    return false;

  if(memcmp(meta_header, "BLITTYPE", 8) == 0) {
    if(read_file(file, meta_offset + 8, sizeof(RawTypeMetadata), (char *)&type_meta) != sizeof(RawTypeMetadata))
      return false;
  }

  return true;
}

static uint32_t find_installed_blit(RawMetadata &meta) {
  for(uint32_t off = 0; off < flash_end;) {
    auto size = get_installed_file_size(off);

    if(!size) {
      off += game_block_size;
      continue;
    }

    auto header = (BlitGameHeader *)(FLASH_BASE + off);
    auto flash_meta = (RawMetadata *)(FLASH_BASE + off + header->end + 10);

    // check CRC ant title
    if(meta.crc32 == flash_meta->crc32 && strcmp(meta.title, flash_meta->title) == 0) {
      return off;
    }

    off += calc_num_blocks(size) * game_block_size;
  }

  return ~0u;
}

// returns lowercased extension
static std::string get_file_ext(const char *path) {
  // get the extension
  std::string_view sv(path);
  auto last_dot = sv.find_last_of('.');
  auto ext = last_dot == std::string::npos ? "" : std::string(sv.substr(last_dot + 1));
  for(auto &c : ext)
    c = tolower(c);

  return ext;
}

static bool cleanup_duplicates(RawMetadata &meta, uint32_t new_offset) {
  bool ret = false;
  for(uint32_t off = 0; off < flash_end;) {
    auto size = get_installed_file_size(off);

    if(!size) {
      off += game_block_size;
      continue;
    }

    auto header = (BlitGameHeader *)(FLASH_BASE + off);
    auto flash_meta = (RawMetadata *)(FLASH_BASE + off + header->end + 10);

    // check title and author, ignore the current copy of the game
    if(off != new_offset && strcmp(meta.title, flash_meta->title) == 0 && strcmp(meta.author, flash_meta->author) == 0) {
      erase_game(off);

      if(off == current_game_offset)
        ret = true;
    }

    off += calc_num_blocks(size) * game_block_size;
  }

  return ret;
}

#endif

// 32blit API

RawMetadata *get_running_game_metadata() {
#ifdef BUILD_LOADER
  if(!current_game_offset)
    return nullptr;

  auto game_ptr = reinterpret_cast<uint8_t *>(FLASH_BASE + current_game_offset);

  auto header = reinterpret_cast<BlitGameHeader *>(game_ptr);

  if(header->magic == blit_game_magic) {
    auto end_ptr = game_ptr + header->end;
    if(memcmp(end_ptr, "BLITMETA", 8) == 0)
      return reinterpret_cast<RawMetadata *>(end_ptr + 10);
  }

#endif
  return nullptr;
}

void create_type_handler_list() {
  handlers.clear();

  blit::api.list_installed_games([](const uint8_t *ptr, uint32_t block, uint32_t size) {
    auto header = (const BlitGameHeader *)ptr;
    auto metadata_ptr = ptr + header->end;

    if(memcmp(metadata_ptr, "BLITMETA", 8) != 0)
      return;

    // skip straight to the type block
    metadata_ptr += 10 + sizeof(RawMetadata);

    if(memcmp(metadata_ptr, "BLITTYPE", 8) != 0)
      return;

    // get the type block
    auto type_meta = (const RawTypeMetadata *)(metadata_ptr + 8);

    // loop through filetypes
    for(unsigned i = 0; i < type_meta->num_filetypes; i++) {
      TypeHandlerInfo info;
      memcpy(info.type, type_meta->filetypes[i], 4);
      info.offset = block * game_block_size;

      handlers.emplace_front(info);
    }
  });
}

bool launch_file(const char *path) {
#ifdef BUILD_LOADER
  uint32_t flash_offset = ~0u;

  // games installed or deleted since boot can add or remove handlers
  create_type_handler_list();

  // clear old path
  launch_path[0] = 0;

  if(strncmp(path, "flash:/", 7) == 0) // from flash
    flash_offset = atoi(path + 7) * game_block_size;
  else {

    auto ext = get_file_ext(path);

    if(ext != "blit") {
      // with handler
      for(auto &handler : handlers) {
        if(strncmp(ext.data(), handler.type, 4) == 0) {
          flash_offset = handler.offset;
          break;
        }
      }

      // set launch path
      strncpy(launch_path, path, sizeof(launch_path) - 1);
    } else {
      // from storage
      auto file = open_file(path, blit::OpenMode::read);

      if(!file)
        return false;

      // read file metadata and try to find matching installed gat
      RawMetadata meta;
      RawTypeMetadata type_meta = {};

      if(read_file_metadata(file, meta, type_meta))
        flash_offset = find_installed_blit(meta);

      // flash if not found
      if(flash_offset == ~0u) {
        BlitWriter writer;

        uint32_t file_offset = 0;
        uint32_t len = get_file_length(file);

        writer.init(len);

        // read in small chunks
        uint8_t buf[FLASH_PAGE_SIZE];

        while(file_offset < len) {
          auto bytes_read = read_file(file, file_offset, FLASH_PAGE_SIZE, (char *)buf);
          if(bytes_read <= 0)
            break;

          if(!writer.write(buf, bytes_read))
            break;

          file_offset += bytes_read;
        }

        close_file(file);

        // didn't write everything, fail launch
        if(writer.get_remaining() > 0)
          return false;

        flash_offset = writer.get_flash_offset();

        cleanup_duplicates(meta, flash_offset);
      } else
        close_file(file);
    }
  }

  // no installed app plays this file type (or the .blit wasn't found)
  if(flash_offset == ~0u) {
    launch_path[0] = 0;
    return false;
  }

  auto header = (BlitGameHeader *)(FLASH_BASE + flash_offset);
  // check header magic + device
  if(header->magic != blit_game_magic || header->device_id != DEVICE_ID)
    return false;

  if(!header->init || !header->render || !header->tick)
    return false;

  requested_launch_offset = flash_offset;
  return true;
#else
  return false;
#endif
}

blit::CanLaunchResult can_launch(const char *path) {
#ifdef BUILD_LOADER
  if(strncmp(path, "flash:/", 7) == 0) {
    // assume anything flashed is compatible for now
    return blit::CanLaunchResult::Success;
  }

  // get the extension
  auto ext = get_file_ext(path);

  if(ext != "blit")
    create_type_handler_list();

  if(ext == "blit") {
    BlitGameHeader header;
    auto file = open_file(path, blit::OpenMode::read);

    if(!file)
      return blit::CanLaunchResult::InvalidFile;

    auto bytes_read = read_file(file, 0, sizeof(header), (char *)&header);

    if(bytes_read == sizeof(header) && header.magic == blit_game_magic && header.device_id == DEVICE_ID) {
      close_file(file);
      return blit::CanLaunchResult::Success;
    }

    close_file(file);
    return blit::CanLaunchResult::IncompatibleBlit;
  }

  // check handlers
  for(auto &handler : handlers) {
    if(strncmp(ext.data(), handler.type, 4) == 0) {
      return blit::CanLaunchResult::Success;
    }
  }

#endif

  return blit::CanLaunchResult::UnknownType;
}

void launch_pre_init() {
  // reset api state before launching new game
  blit::api_data.buttons = {0, 0, 0};

  blit::api_data.LED = {0, 0, 0};
  blit::api_data.vibration = 0.0f;
  blit::api_data.message_received = nullptr;
  blit::api_data.i2c_completed = nullptr;
  blit::api_data.on_quit = nullptr;

  for(int i = 0; i < CHANNEL_COUNT; i++)
    blit::api.channels[i] = blit::AudioChannel();
}

void delayed_launch() {
  if(!requested_launch_offset)
    return;

  auto header = (BlitGameHeader *)(FLASH_BASE + requested_launch_offset);

#ifdef PICO_RP2350
  uint32_t header_offset = *(uint32_t *)(FLASH_BASE + requested_launch_offset + sizeof(BlitGameHeader));
  if(header_offset != requested_launch_offset) {
    // setup translation
    uint32_t size = 4 * 1024 * 1024; // TODO: use (rounded) blit size
    qmi_hw->atrans[1] = (size >> 12) << QMI_ATRANS1_SIZE_LSB
                      | (requested_launch_offset >> 12) << QMI_ATRANS1_BASE_LSB;

    // invalidate cache
    xip_cache_invalidate_range(4 * 1024 * 1024, size);

    // FIXME: handle previous blit also using translation on failure
  }
#endif

  // save in case launch fails
  uint32_t last_game_offset = current_game_offset;

  current_game_offset = requested_launch_offset;
  requested_launch_offset = 0;

  launch_pre_init();

  if(!header->init(0)) {
    blit::debugf("failed to init game!\n");
    current_game_offset = last_game_offset;
    return;
  }

  blit::render = header->render;
  do_tick = header->tick;
}

void list_installed_games(std::function<void(const uint8_t *, uint32_t, uint32_t)> callback) {
  for(uint32_t off = 0; off < flash_end;) {
    auto size = get_installed_file_size(off);

    if(!size) {
      off += game_block_size;
      continue;
    }

    callback((const uint8_t *)(FLASH_BASE + off), off / game_block_size, size);

    off += calc_num_blocks(size) * game_block_size;
  }
}

void erase_game(uint32_t offset) {
#ifdef BUILD_LOADER
  // check alignment
  if(offset & (game_block_size - 1))
    return;

  // check in bounds
  if(offset >= flash_end)
    return;

  auto size = get_installed_file_size(offset);

  // fall back to one block if size unknown
  auto num_blocks = size == 0 ? 1 : calc_num_blocks(size);

  // we can't display progress unless we break up the erase...
  set_render_overlay_enabled(true);
  set_overlay_message("Deleting game...");
  set_overlay_progress(0, 0);
  // make sure the message is displayed before we disable interrupts/core1
  overlay_try_render(true);

  // do erase
  auto status = save_and_disable_interrupts();

  if(core1_started)
    multicore_lockout_start_blocking(); // pause core1

  // the real erase size is smaller than the one baked into the API...
  static_assert(game_block_size % FLASH_SECTOR_SIZE == 0);
  flash_range_erase(offset, num_blocks * game_block_size);

  if(core1_started)
    multicore_lockout_end_blocking(); // resume core1

  restore_interrupts(status);

  set_render_overlay_enabled(false);

  // clear out any handler entries
  auto prev = handlers.before_begin();
  for(auto it = handlers.begin(); it != handlers.end();) {
    if(it->offset == offset)
      it = handlers.erase_after(prev);
    else
      prev = it++;
  }
#endif
}

void *get_type_handler_metadata(const char *filetype) {
#ifdef BUILD_LOADER
  for(auto &handler : handlers) {
    if(strncmp(filetype, handler.type, 4) == 0) {
      auto ptr = (const uint8_t *)(FLASH_BASE + handler.offset);
      auto header = (const BlitGameHeader *)ptr;
      auto metadata_ptr = ptr + header->end;

      // this really shouldn't fail if it's in the list, but be safe
      if(memcmp(metadata_ptr, "BLITMETA", 8) == 0)
        return (void *)metadata_ptr;
    }
  }
#endif
  return nullptr;
}

const char *get_launch_path() {
#ifdef BUILD_LOADER
  if(!launch_path[0])
    return nullptr;

  return launch_path;
#else
  return nullptr;
#endif
}

void get_app_flash_usage(uint32_t &total, uint32_t &used) {
  // the loader is in the first 256k
  total = flash_end - 256 * 1024;
  used = 0;

  list_installed_games([&used](const uint8_t *, uint32_t, uint32_t size) {
    used += calc_num_blocks(size) * game_block_size;
  });
}

bool is_launcher_running() {
#ifdef BUILD_LOADER
  // about to launch something
  if(requested_launch_offset)
    return false;

  // nothing launched, only the loader is running
  if(!current_game_offset)
    return true;

  auto meta = get_running_game_metadata();
  if(!meta)
    return false;

  auto type_ptr = reinterpret_cast<const char *>(meta + 1);
  if(memcmp(type_ptr, "BLITTYPE", 8) != 0)
    return false;

  auto type_meta = reinterpret_cast<const RawTypeMetadata *>(type_ptr + 8);
  return strncmp(type_meta->category, "launcher", sizeof(type_meta->category)) == 0;
#else
  return false;
#endif
}

#ifdef BUILD_LOADER
static uint32_t crc32_update(uint32_t crc, const uint8_t *data, uint32_t len) {
  crc = ~crc;
  while(len--) {
    crc ^= *data++;
    for(int i = 0; i < 8; i++)
      crc = (crc >> 1) ^ (0xEDB88320 & -(crc & 1));
  }
  return ~crc;
}
#endif

#ifdef BUILD_LOADER
static bool is_launcher_at(uint32_t offset) {
  auto header = (const BlitGameHeader *)(FLASH_BASE + offset);
  auto meta = (const char *)(FLASH_BASE + offset + header->end);

  if(memcmp(meta, "BLITMETA", 8) != 0 || memcmp(meta + 10 + sizeof(RawMetadata), "BLITTYPE", 8) != 0)
    return false;

  auto type_meta = (const RawTypeMetadata *)(meta + 10 + sizeof(RawMetadata) + 8);
  return strncmp(type_meta->category, "launcher", sizeof(type_meta->category)) == 0;
}

static void erase_other_launchers(uint32_t keep_offset) {
  for(uint32_t off = 0; off < flash_end;) {
    auto size = get_installed_file_size(off);

    if(!size) {
      off += game_block_size;
      continue;
    }

    if(off != keep_offset && is_launcher_at(off))
      erase_game(off);

    off += calc_num_blocks(size) * game_block_size;
  }
}
#endif

InstallResult install_blit_file(const char *path, std::function<void(uint32_t done, uint32_t total)> progress) {
#ifdef BUILD_LOADER
  auto file = open_file(path, blit::OpenMode::read);
  if(!file)
    return InstallResult::Failed;

  auto fail = [file](InstallResult res) {
    close_file(file);
    return res;
  };

  uint32_t len = get_file_length(file);

  // header + the base address that follows it
  uint8_t header_buf[sizeof(BlitGameHeader) + 4];
  if(read_file(file, 0, sizeof(header_buf), (char *)header_buf) != sizeof(header_buf))
    return fail(InstallResult::NotAGame);

  auto &header = *(BlitGameHeader *)header_buf;

  if(header.magic != blit_game_magic || header.device_id != DEVICE_ID)
    return fail(InstallResult::NotAGame);

  if(header.api_version_major != blit::api_version_major || header.api_version_minor > blit::api_version_minor)
    return fail(InstallResult::NeedsUpdate);

  if(header.end < sizeof(header_buf) || header.end > len)
    return fail(InstallResult::Damaged);

  // metadata is optional, but when it's there it has to be complete
  RawMetadata meta;
  RawTypeMetadata type_meta = {};
  bool have_meta = read_file_metadata(file, meta, type_meta);

  if(have_meta) {
    uint16_t meta_len = 0;
    read_file(file, header.end + 8, 2, (char *)&meta_len);
    if(header.end + 10 + meta_len > len)
      return fail(InstallResult::Damaged);
  }

  static uint8_t buf[FLASH_PAGE_SIZE];

  // the metadata checksum covers the code and data before it
  if(have_meta) {
    uint32_t crc = 0;
    for(uint32_t off = 0; off < header.end;) {
      auto chunk = std::min(uint32_t(FLASH_PAGE_SIZE), header.end - off);
      if(read_file(file, off, chunk, (char *)buf) != int32_t(chunk))
        return fail(InstallResult::Failed);

      crc = crc32_update(crc, buf, chunk);
      off += chunk;
    }

    if(crc != meta.crc32)
      return fail(InstallResult::Damaged);

    // this exact build is already installed
    if(find_installed_blit(meta) != ~0u)
      return fail(InstallResult::AlreadyInstalled);
  }

#ifdef PICO_RP2350
  // relocatable builds go in any free space, make room by removing the old copy if needed
  uint32_t base = *(uint32_t *)(header_buf + sizeof(BlitGameHeader)) & 0xFFFFFF;
  if(base == 4 * 1024 * 1024 && !find_flash_offset(len)) {
    if(have_meta)
      cleanup_duplicates(meta, 0);

    if(!find_flash_offset(len))
      return fail(InstallResult::NoSpace);
  }
#endif

  BlitWriter writer;
  writer.init(len);

  for(uint32_t off = 0; off < len;) {
    auto chunk = std::min(uint32_t(FLASH_PAGE_SIZE), len - off);
    memset(buf, 0xFF, sizeof(buf));

    if(read_file(file, off, chunk, (char *)buf) != int32_t(chunk))
      break;

    if(!writer.write(buf, chunk))
      break;

    off += chunk;

    if(progress && (off % (FLASH_PAGE_SIZE * 16) == 0 || off == len))
      progress(off, len);
  }

  close_file(file);

  if(writer.get_remaining() > 0) {
    // don't leave a partial copy behind
    if(writer.get_flash_offset())
      erase_game(writer.get_flash_offset());
    return InstallResult::Failed;
  }

  auto flash_offset = writer.get_flash_offset();

  // check what ended up in flash
  if(have_meta && crc32_update(0, (const uint8_t *)(FLASH_BASE + flash_offset), header.end) != meta.crc32) {
    erase_game(flash_offset);
    return InstallResult::Failed;
  }

  if(have_meta)
    cleanup_duplicates(meta, flash_offset);

  // there's one home screen, a new launcher replaces any other
  if(strncmp(type_meta.category, "launcher", sizeof(type_meta.category)) == 0)
    erase_other_launchers(flash_offset);

  return InstallResult::Installed;
#else
  return InstallResult::Failed;
#endif
}

#ifdef BUILD_LOADER

// .blit file writer
void BlitWriter::init(uint32_t file_len) {
  this->file_len = file_len;
  file_offset = flash_offset = 0;
}

bool BlitWriter::write(const uint8_t *buf, uint32_t len) {
  if(!flash_offset) {
    if(!prepare_write(buf))
      return false;
  }

  if(file_offset >= file_len)
    return false;

  // write page
  auto status = save_and_disable_interrupts();

  if(core1_started)
    multicore_lockout_start_blocking(); // pause core1

  // assuming len <= page size and buf size == page size
  flash_range_program(flash_offset + file_offset, buf, FLASH_PAGE_SIZE);

  if(core1_started)
    multicore_lockout_end_blocking(); // resume core1

  restore_interrupts(status);

  file_offset += len;

  return true;
}

void BlitWriter::cleanup_duplicates() {
  auto header = (BlitGameHeader *)(FLASH_BASE + flash_offset);
  auto meta = (RawMetadata *)(FLASH_BASE + flash_offset + header->end + 10);
  ::cleanup_duplicates(*meta, flash_offset);
}

uint32_t BlitWriter::get_offset() const {
  return file_offset;
}

uint32_t BlitWriter::get_length() const {
  return file_len;
}

uint32_t BlitWriter::get_remaining() const {
  return file_len - file_offset;
}

uint32_t BlitWriter::get_flash_offset() const {
  return flash_offset;
}

bool BlitWriter::prepare_write(const uint8_t *buf) {
  auto header = (BlitGameHeader *)buf;

  if(header->magic != blit_game_magic || header->device_id != DEVICE_ID) {
    blit::debugf("Invalid blit header!");
    return false;
  }

  // currently non-relocatable, so base address is stored after header
  flash_offset = *(uint32_t *)(buf + sizeof(BlitGameHeader));
  flash_offset &= 0xFFFFFF;

#ifdef PICO_RP2350
  if(flash_offset == 4 * 1024 * 1024) {
    // we can use address translation for this, so flash in any free space
    flash_offset = find_flash_offset(file_len);
  }

  if(!flash_offset) {
    blit::debugf("Not enough space!");
    return false;
  }
#endif

  disable_user_code();

  // erase flash
  auto status = save_and_disable_interrupts();

  if(core1_started)
    multicore_lockout_start_blocking(); // pause core1

  auto erase_size = ((file_len - 1) / FLASH_SECTOR_SIZE) + 1;
  flash_range_erase(flash_offset, erase_size * FLASH_SECTOR_SIZE);

  if(core1_started)
    multicore_lockout_end_blocking(); // resume core1

  restore_interrupts(status);

  return true;
}

#endif
