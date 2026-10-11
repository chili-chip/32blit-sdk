#include <cstdint>
#include <cstring>
#include <functional>
#include <vector>

#include "pico/binary_info.h"
#include "pico/time.h"

#include "ff.h"
#include "diskio.h"

#include "blit_launch.hpp"
#include "file.hpp"
#include "storage.hpp"
#include "executable.hpp"
#include "config.h"

#if defined(AUTO_INSTALL) && defined(BUILD_LOADER)
#include "installer.hpp"
#endif

static FATFS fs;
static bool initialised = false;

// the USB host wrote to storage behind fatfs' back, remount once it goes quiet
static bool host_wrote = false;
static absolute_time_t host_write_time;
static const uint32_t host_write_settle_ms = 500;

// fatfs io funcs
DSTATUS disk_initialize(BYTE pdrv) {
  initialised = storage_init();
  return initialised ? RES_OK : STA_NOINIT;
}

DSTATUS disk_status(BYTE pdrv) {
  return initialised ? RES_OK : STA_NOINIT;
}

DRESULT disk_read(BYTE pdrv, BYTE *buff, LBA_t sector, UINT count) {
  static_assert(FF_MIN_SS == FF_MAX_SS);
  return storage_read(sector, 0, buff, FF_MIN_SS * count) == int32_t(FF_MIN_SS * count) ? RES_OK : RES_ERROR;
}

DRESULT disk_write(BYTE pdrv, const BYTE *buff, LBA_t sector, UINT count) {
  return storage_write(sector, 0, buff, FF_MIN_SS * count) == int32_t(FF_MIN_SS * count) ? RES_OK : RES_ERROR;
}

DRESULT disk_ioctl(BYTE pdrv, BYTE cmd, void* buff) {
  uint16_t block_size;
  uint32_t num_blocks;

  switch(cmd) {
    case CTRL_SYNC:
      return RES_OK;

    case GET_SECTOR_COUNT:
      get_storage_size(block_size, num_blocks);
      *(LBA_t *)buff = num_blocks;
      return RES_OK;

    case GET_BLOCK_SIZE:
      *(DWORD *)buff = 1;
      return RES_OK;
  }

  return RES_PARERR;
}

void init_fs() {
  auto res = f_mount(&fs, "", 1);

  // auto-format flash, but not SD cards
#ifndef STORAGE_SD
  if(res == FR_NO_FILESYSTEM) {
    printf("No filesystem found, formatting...\n");

    MKFS_PARM opts{};
    opts.fmt = FM_ANY | FM_SFD;
    res = f_mkfs("", &opts, fs.win, FF_MAX_SS);

    if(res != FR_OK) {
      printf("...failed! (%i)\n", res);
      return;
    }

    res = f_mount(&fs, "", 1);
  }
#endif

  if(res != FR_OK) {
    printf("Failed to mount filesystem! (%i)\n", res);
    return;
  }

#ifdef STORAGE_VOLUME_LABEL
  // name the drive, also relabels drives formatted before the name was set
  char label[12];
  if(f_getlabel("", label, nullptr) == FR_OK && strcmp(label, STORAGE_VOLUME_LABEL) != 0)
    f_setlabel(STORAGE_VOLUME_LABEL);
#endif
}

void storage_host_written() {
  host_wrote = true;
  host_write_time = get_absolute_time();

#if defined(AUTO_INSTALL) && defined(BUILD_LOADER)
  installer_request_scan();
#endif
}

void update_fs() {
  if(!host_wrote || get_files_open())
    return;

  if(absolute_time_diff_us(host_write_time, get_absolute_time()) < host_write_settle_ms * 1000)
    return;

  // drop cached sectors and re-read the volume
  auto res = f_mount(&fs, "", 1);
  if(res != FR_OK)
    printf("Failed to remount filesystem! (%i)\n", res);

  host_wrote = false;
}

bool is_fs_available() {
  // report a change while the host is writing so a launcher re-scans after the remount
  return !host_wrote && is_storage_available();
}

static char save_path[32]; // max game title length is 24 + ".blit/" + "/"

const char *get_save_path() {
  const char *app_name = "_unknown";

  if(!directory_exists(".blit"))
    create_directory(".blit");

  app_name = "_unknown";

  if(auto meta = get_running_game_metadata())
    app_name = meta->title;
  else {
    // find the program name in the binary info
    extern binary_info_t *__binary_info_start, *__binary_info_end;

    for(auto tag_ptr = &__binary_info_start; tag_ptr != &__binary_info_end ; tag_ptr++) {
      if((*tag_ptr)->type != BINARY_INFO_TYPE_ID_AND_STRING || (*tag_ptr)->tag != BINARY_INFO_TAG_RASPBERRY_PI)
        continue;

      auto id_str_tag = (binary_info_id_and_string_t *)*tag_ptr;

      if(id_str_tag->id == BINARY_INFO_ID_RP_PROGRAM_NAME) {
        app_name = id_str_tag->value;
        break;
      }
    }
  }

  snprintf(save_path, sizeof(save_path), ".blit/%s/", app_name);

  // make sure it exists
  if(!directory_exists(save_path))
    create_directory(save_path);


  return save_path;
}
