// Sandbox for .blit games on RP2350 (32blit-sdk #91, BLIT_SANDBOX).
//
// A .blit game is native code. Without a sandbox it runs privileged on the
// loader's stack and can reach flash, the loader's memory and every
// peripheral. With BLIT_SANDBOX the loader runs each game call (init, tick,
// render, on_quit, message_received) in unprivileged Thread mode on its own
// stack (PSP), with the Armv8-M MPU allowing only:
//
//   region 0  loader flash            read, execute (API trampolines, APIConst)
//   region 1  the game's flash image  read, execute
//   region 2  the installed copy      read (when the image is address-translated)
//   region 3  shared RAM              read, write (APIData, audio channels, copies)
//   region 4  the game's stack        read, write
//   region 5  the game's RAM          read, write, execute
//
// Everything else (other games, the loader's RAM, peripherals, the PPB) faults,
// and the fault stops the game instead of the console. The game reaches the
// loader only through `svc` (sandbox_gate.S); sandbox_check_call checks every
// pointer it passes and refuses the calls a game has no reason to make.
// Privileged code keeps the default memory map (MPU_CTRL.PRIVDEFENA).
//
// The home launcher (category "launcher") is trusted: privileged, no MPU.
// Arm documentation: Armv8-M Architecture Reference Manual (DDI0553) B3 and
// "Armv8-M Memory Protection Unit" (100699); RP2350 datasheet 3.7 (Cortex-M33).

#include "sandbox.hpp"

#if BLIT_SANDBOX_ENABLED

#include <cstddef>
#include <cstring>
#include <new>
#include <string>

#include "hardware/exception.h"
#include "hardware/regs/addressmap.h"
#include "hardware/structs/m33.h"
#include "hardware/structs/mpu.h"
#include "hardware/structs/scb.h"
#include "hardware/sync.h"
#include "pico/platform/panic.h"

#include "audio/audio.hpp"
#include "engine/api_private.hpp"
#include "engine/version.hpp"

#include "sandbox_abi.h"

static_assert(offsetof(blit::APIConst, set_screen_mode) == 8, "APIConst: version and channels first");
static_assert(sizeof(blit::APIConst) == 8 + 4 * SANDBOX_API_SLOTS, "SANDBOX_API_SLOTS must match APIConst");

// APIConst slots, in order (32blit/engine/api_private.hpp)
enum Slot : uint32_t {
  set_screen_mode, set_screen_palette, now, random, exit_game, debug,
  open_file, read_file, write_file, close_file, get_file_length, list_files,
  file_exists, directory_exists, create_directory, rename_file, remove_file,
  get_save_path, is_storage_available, enable_us_timer, get_us_timer, get_max_us_timer,
  decode_jpeg_buffer, decode_jpeg_file, launch, erase_game, get_type_handler_metadata,
  get_launch_path, is_multiplayer_connected, set_multiplayer_enabled, send_message,
  flash_to_tmp, tmp_file_closed, get_metadata, set_screen_mode_format,
  i2c_send, i2c_receive, set_raw_cdc_enabled, cdc_write, cdc_read,
  list_installed_games, can_launch, get_screen_data, set_framebuffer,
  get_battery_level, get_battery_voltage, is_usb_connected,
  get_system_settings, set_system_settings, get_system_info,
  slot_count
};
static_assert(slot_count == SANDBOX_API_SLOTS);

// from the linker script
extern "C" char __api_data_start[], __sandbox_shared_end[];
extern "C" char __StackLimit[], __StackTop[];
extern "C" char __flash_binary_end[];

// 32blit/audio/audio.cpp: the game's wave buffer callbacks would run
// privileged on core 1, outside the MPU, so a sandboxed game gets none.
namespace blit { extern volatile bool block_wave_buffer_callbacks; }

// from sandbox_gate.S
extern "C" uint32_t sandbox_enter(const void *function, uint32_t arg0, uint32_t arg1, uint32_t stack_top);
extern "C" void sandbox_exit();
extern "C" void sandbox_api_dispatch();

// The APIConst games see, at 0x10000200: the same version and channels as
// the loader's own (blit::api), and a trampoline for each function the loader
// has (sandbox_init checks that they match).
extern "C" void sandbox_api_0(), sandbox_api_1(), sandbox_api_2(), sandbox_api_3(), sandbox_api_4(),
  sandbox_api_5(), sandbox_api_6(), sandbox_api_7(), sandbox_api_8(), sandbox_api_9(),
  sandbox_api_10(), sandbox_api_11(), sandbox_api_12(), sandbox_api_13(), sandbox_api_14(),
  sandbox_api_15(), sandbox_api_16(), sandbox_api_17(), sandbox_api_18(), sandbox_api_19(),
  sandbox_api_20(), sandbox_api_21(), sandbox_api_22(), sandbox_api_23(), sandbox_api_24(),
  sandbox_api_25(), sandbox_api_26(), sandbox_api_27(), sandbox_api_28(), sandbox_api_29(),
  sandbox_api_30(), sandbox_api_31(), sandbox_api_32(), sandbox_api_33(), sandbox_api_34(),
  sandbox_api_35(), sandbox_api_36(), sandbox_api_37(), sandbox_api_38(), sandbox_api_39(),
  sandbox_api_40(), sandbox_api_41(), sandbox_api_42(), sandbox_api_43(), sandbox_api_44(),
  sandbox_api_45(), sandbox_api_46(), sandbox_api_47(), sandbox_api_48(), sandbox_api_49();

struct GameApiConst {
  uint16_t version_major, version_minor;
  blit::AudioChannel *channels;
  const void *slots[SANDBOX_API_SLOTS];
};
static_assert(sizeof(GameApiConst) == sizeof(blit::APIConst));

extern uint8_t sandbox_channel_storage[]; // main.cpp

#define G(n) reinterpret_cast<const void *>(&sandbox_api_##n)
[[gnu::section(".rodata.api_const"), gnu::used]]
static const GameApiConst game_api_const {
  blit::api_version_major, blit::api_version_minor,
  reinterpret_cast<blit::AudioChannel *>(sandbox_channel_storage),
  {
    G(0), G(1), G(2), G(3), G(4), G(5), G(6), G(7),
    G(8), G(9), G(10), G(11), G(12), G(13), G(14), G(15),
    G(16), G(17), G(18), nullptr, G(20), G(21), nullptr, nullptr,
    G(24), G(25), G(26), G(27), G(28), G(29), G(30), nullptr,
    nullptr, G(33), G(34), nullptr, nullptr, nullptr, nullptr, nullptr,
    G(40), G(41), G(42), G(43), G(44), G(45), G(46), G(47),
    G(48), G(49),
  }
};
#undef G

// CONTROL[1:0] while game code runs: SPSEL (PSP), plus nPRIV unless trusted
extern "C" { volatile uint32_t sandbox_thread_control = 3; }

// The loader keeps the top of its stack area for itself, the game gets the rest.
static constexpr uint32_t loader_stack_size = 8 * 1024;

// where loader flash starts and game RAM ends (sandbox_test/ runs this in QEMU elsewhere)
#ifndef SANDBOX_LOADER_FLASH_START
#define SANDBOX_LOADER_FLASH_START XIP_BASE
#endif
#ifndef SANDBOX_GAME_RAM_END
#define SANDBOX_GAME_RAM_END SRAM_STRIPED_END
#endif

// Copies the game may read: the loader's own data stays out of its reach.
struct Shared {
  blit::SurfaceInfo surface;
  blit::Pen palette[256];
  char save_path[128];
  char launch_path[128];
  uint8_t message[512];
};

[[gnu::section(".bss.sandbox_shared.copies")]]
alignas(Shared) static uint8_t shared_storage[sizeof(Shared)];
static Shared &shared = *reinterpret_cast<Shared *>(shared_storage);

struct Range {
  uintptr_t start = 0, end = 0;

  bool contains(uintptr_t p, size_t len) const {
    return p >= start && p <= end && len <= end - p;
  }
};

static Range loader_flash, shared_ram, game_stack, game_ram;

static struct {
  bool active = false;
  bool trusted = false;
  Range image, installed;
  const void *tick = nullptr, *render = nullptr;
} game;

static volatile bool in_call = false, in_api = false;
static const char *volatile stop_reason = nullptr;
static void *open_files[16];

static bool game_writable(const void *p, size_t len) {
  auto a = uintptr_t(p);
  return game_ram.contains(a, len) || game_stack.contains(a, len) || shared_ram.contains(a, len);
}

static bool game_readable(const void *p, size_t len) {
  auto a = uintptr_t(p);
  return game_writable(p, len) || game.image.contains(a, len) || game.installed.contains(a, len)
      || loader_flash.contains(a, len);
}

static bool game_string(const char *s, size_t max = 256) {
  for(size_t i = 0; i < max; i++) {
    if(!game_readable(s + i, 1))
      return false;
    if(!s[i])
      return true;
  }
  return false;
}

static bool game_std_string(const void *p) {
  if(!game_readable(p, sizeof(std::string)))
    return false;
  auto s = static_cast<const std::string *>(p);
  return s->size() < 256 && game_readable(s->data(), s->size() + 1);
}

static bool game_file(void *fh) {
  if(!fh)
    return false;
  for(auto f : open_files)
    if(f == fh)
      return true;
  return false;
}

static bool game_framebuffer(const void *p, size_t len) {
  return game_ram.contains(uintptr_t(p), len);
}

// The calls with a result the game must not get as is.

static blit::SurfaceInfo &sandboxed_set_screen_mode(blit::ScreenMode mode) {
  auto &info = blit::api.set_screen_mode(mode);
  shared.surface = info;
  if(info.palette && !game_readable(info.palette, sizeof(shared.palette))) {
    memcpy(shared.palette, info.palette, sizeof(shared.palette));
    shared.surface.palette = shared.palette;
  }
  return shared.surface;
}

static void *sandboxed_open_file(const std::string &name, int mode) {
  int free_slot = -1;
  for(int i = 0; i < int(std::size(open_files)); i++) {
    if(!open_files[i]) {
      free_slot = i;
      break;
    }
  }
  if(free_slot < 0)
    return nullptr;

  auto fh = blit::api.open_file(name, mode);
  open_files[free_slot] = fh;
  return fh;
}

static int32_t sandboxed_close_file(void *fh) {
  for(auto &f : open_files)
    if(f == fh)
      f = nullptr;
  return blit::api.close_file(fh);
}

static const char *copy_path(char *to, size_t size, const char *from) {
  if(!from)
    return nullptr;
  strncpy(to, from, size - 1);
  to[size - 1] = 0;
  return to;
}

static const char *sandboxed_get_save_path() {
  return copy_path(shared.save_path, sizeof(shared.save_path), blit::api.get_save_path());
}

static const char *sandboxed_get_launch_path() {
  return copy_path(shared.launch_path, sizeof(shared.launch_path), blit::api.get_launch_path());
}

static constexpr uintptr_t refuse = 0, stop = 1;

static uintptr_t stop_call(const char *reason) {
  if(!stop_reason)
    stop_reason = reason;
  return stop;
}

static uintptr_t real(uint32_t slot) {
  return reinterpret_cast<const uintptr_t *>(reinterpret_cast<const char *>(&blit::api) + 8)[slot];
}

// Called by sandbox_api_dispatch, privileged, with the game's r0-r3.
extern "C" uintptr_t sandbox_check_call(const uint32_t *a, uint32_t slot) {
  in_api = true;

  if(slot >= slot_count || !real(slot))
    return refuse;

  if(game.trusted)
    return real(slot);

  const char *bad_pointer = "passed the API memory that is not its own";
  auto ptr = [a](int i) { return reinterpret_cast<void *>(a[i]); };

  switch(Slot(slot)) {
    // no pointers
    case now: case random: case exit_game: case is_storage_available:
    case get_us_timer: case get_max_us_timer: case is_multiplayer_connected:
    case set_multiplayer_enabled: case get_screen_data: case get_battery_level:
    case get_battery_voltage: case is_usb_connected:
      return real(slot);

    case set_screen_mode:
      return uintptr_t(sandboxed_set_screen_mode);

    case set_screen_palette:
      if(a[1] > 256 || !game_readable(ptr(0), a[1] * sizeof(blit::Pen)))
        return stop_call(bad_pointer);
      return real(slot);

    case debug:
    case can_launch:
      return game_string(static_cast<const char *>(ptr(0))) ? real(slot) : stop_call(bad_pointer);

    case open_file:
      return game_std_string(ptr(0)) ? uintptr_t(sandboxed_open_file) : stop_call(bad_pointer);

    case file_exists: case directory_exists: case create_directory: case remove_file:
      return game_std_string(ptr(0)) ? real(slot) : stop_call(bad_pointer);

    case rename_file:
      return game_std_string(ptr(0)) && game_std_string(ptr(1)) ? real(slot) : stop_call(bad_pointer);

    case read_file:
      if(!game_file(ptr(0)) || !game_writable(ptr(3), a[2]))
        return stop_call(bad_pointer);
      return real(slot);

    case write_file:
      if(!game_file(ptr(0)) || !game_readable(ptr(3), a[2]))
        return stop_call(bad_pointer);
      return real(slot);

    case close_file:
      return game_file(ptr(0)) ? uintptr_t(sandboxed_close_file) : stop_call(bad_pointer);

    case get_file_length:
      return game_file(ptr(0)) ? real(slot) : stop_call(bad_pointer);

    case get_save_path:
      return uintptr_t(sandboxed_get_save_path);

    case get_launch_path:
      return uintptr_t(sandboxed_get_launch_path);

    case send_message:
      return game_readable(ptr(0), a[1] & 0xFFFF) ? real(slot) : stop_call(bad_pointer);

    case get_metadata: // returned through r0
      return game_writable(ptr(0), sizeof(blit::GameMetadata)) ? real(slot) : stop_call(bad_pointer);

    case set_screen_mode_format:
      return game_writable(ptr(1), sizeof(blit::SurfaceTemplate)) ? real(slot) : stop_call(bad_pointer);

    case set_framebuffer: // the loader draws its menus into it
      return game_framebuffer(ptr(0), a[1]) ? real(slot) : stop_call(bad_pointer);

    case get_system_settings:
      return game_writable(ptr(0), sizeof(blit::SystemSettings)) ? real(slot) : stop_call(bad_pointer);

    case get_system_info:
      return game_writable(ptr(0), sizeof(blit::SystemInfo)) ? real(slot) : stop_call(bad_pointer);

    case erase_game:
      return stop_call("tried to erase a game");

    // callbacks into the game would run privileged; other games, settings, raw hardware
    case list_files: case decode_jpeg_buffer: case decode_jpeg_file: case launch:
    case get_type_handler_metadata: case flash_to_tmp: case tmp_file_closed:
    case i2c_send: case i2c_receive: case set_raw_cdc_enabled: case cdc_write: case cdc_read:
    case list_installed_games: case set_system_settings: case enable_us_timer:
    case slot_count:
      return refuse;
  }
  return refuse;
}

// After each API call: a stop requested meanwhile happens now.
extern "C" uint32_t sandbox_api_done() {
  in_api = false;
  return stop_reason != nullptr;
}

static inline void set_privileged() {
  uint32_t control;
  __asm volatile("mrs %0, control" : "=r"(control));
  __asm volatile("msr control, %0" :: "r"(control & ~1u) : "memory");
}

// From an exception taken in the game: return to sandbox_exit, privileged,
// on a fresh frame at the top of the game's stack.
static uint32_t stop_from_handler(uint32_t exc_return, const char *reason) {
  if(!stop_reason)
    stop_reason = reason;

  auto frame = reinterpret_cast<uint32_t *>(game_stack.end - 32);
  for(int i = 0; i < 6; i++)
    frame[i] = 0;
  frame[6] = uintptr_t(sandbox_exit) & ~1u;
  frame[7] = 0x01000000; // Thumb
  __asm volatile("msr psp, %0" :: "r"(frame) : "memory");

  set_privileged();
  m33_hw->fpccr &= ~M33_FPCCR_LSPACT_BITS; // no lazy FP state left on the old stack
  return exc_return | (1u << 4); // EXC_RETURN.FType: a basic frame
}

static bool from_game(uint32_t exc_return) {
  // Thread mode on the process stack, during a call into the game
  return in_call && (exc_return & (1u << 3)) && (exc_return & (1u << 2));
}

extern "C" uint32_t sandbox_svc_c(uint32_t exc_return) {
  if(!from_game(exc_return))
    return exc_return;

  uint32_t *frame;
  __asm volatile("mrs %0, psp" : "=r"(frame));
  uint32_t pc = frame[6];
  uint16_t insn = *reinterpret_cast<const uint16_t *>(pc - 2);
  uint32_t imm = insn & 0xFF;

  if((insn & 0xFF00) != 0xDF00)
    return stop_from_handler(exc_return, "bad svc");

  if(imm == SANDBOX_SVC_RETURN) {
    // privilege comes back only at the gate
    if(pc != (uintptr_t(sandbox_exit) & ~1u))
      return stop_from_handler(exc_return, "svc 255 outside the gate");
    set_privileged();
    return exc_return;
  }

  if(imm < SANDBOX_API_SLOTS) {
    frame[4] = imm; // r12: the slot
    frame[6] = uintptr_t(sandbox_api_dispatch) & ~1u;
    frame[7] = (frame[7] & (1u << 9)) | 0x01000000; // keep the stack-alignment bit, clear IT
    set_privileged();
    return exc_return;
  }

  return stop_from_handler(exc_return, "unknown svc");
}

extern "C" uint32_t sandbox_fault_c(uint32_t exc_return) {
  uint32_t ipsr;
  __asm volatile("mrs %0, ipsr" : "=r"(ipsr));
  auto exception = ipsr & 0x1FF;

  if(exception == PENDSV_EXCEPTION) {
    // a requested stop: now, unless the loader is serving the game (then after the call)
    if(stop_reason && from_game(exc_return) && !in_api)
      return stop_from_handler(exc_return, stop_reason);
    return exc_return;
  }

  if(!from_game(exc_return))
    return 0; // the loader's own fault: stop as before

  const char *reason = "crashed";
  switch(exception) {
    case MEMMANAGE_EXCEPTION: reason = "touched memory that is not its own"; break;
    case BUSFAULT_EXCEPTION:  reason = "bus fault"; break;
    case USAGEFAULT_EXCEPTION: reason = "bad instruction or stack overflow"; break;
  }
  if(in_api)
    reason = "the loader faulted serving an API call";

  // the status bits are write-one-to-clear
  scb_hw->cfsr = scb_hw->cfsr;
  scb_hw->hfsr = scb_hw->hfsr;
  return stop_from_handler(exc_return, reason);
}

static void mpu_region(uint32_t n, const Range &r, uint32_t ap, bool xn) {
  mpu_hw->rnr = n;
  if(r.end <= r.start) {
    mpu_hw->rlar = 0;
    return;
  }
  mpu_hw->rbar = (r.start & M33_MPU_RBAR_BASE_BITS) | (ap << M33_MPU_RBAR_AP_LSB) | (xn ? M33_MPU_RBAR_XN_BITS : 0);
  mpu_hw->rlar = ((r.end - 1) & M33_MPU_RLAR_LIMIT_BITS) | M33_MPU_RLAR_EN_BITS; // attribute 0
}

static constexpr uint32_t ap_rw = 1, ap_ro = 3; // read/write or read-only, any privilege

void sandbox_init() {
  loader_flash = {SANDBOX_LOADER_FLASH_START, (uintptr_t(__flash_binary_end) + 31) & ~31u};
  shared_ram = {uintptr_t(__api_data_start), uintptr_t(__sandbox_shared_end)};
  game_stack = {uintptr_t(__StackLimit), uintptr_t(__StackTop) - loader_stack_size};
  game_ram = {uintptr_t(__StackTop), SANDBOX_GAME_RAM_END};

  // needs the ChiliChip layout: the loader's stack at the top of its RAM, games after it
  if(game_stack.end <= game_stack.start + 8 * 1024 || game_ram.start >= game_ram.end
     || (game_stack.start | game_stack.end | game_ram.start | shared_ram.start | shared_ram.end) & 31)
    panic("sandbox: unsupported loader memory layout");

  // shared RAM is not zeroed at boot
  new (&shared) Shared();

  // a slot the loader leaves empty must be empty for games too
  for(uint32_t slot = 0; slot < slot_count; slot++) {
    if(!real(slot) != !game_api_const.slots[slot])
      panic("sandbox: game APIConst slot %u does not match the loader's", unsigned(slot));
  }

  mpu_hw->ctrl = 0;
  mpu_hw->mair[0] = 0xFF; // attribute 0: normal memory, as the default map has for SRAM
  for(uint32_t n = 0; n < 8; n++) {
    mpu_hw->rnr = n;
    mpu_hw->rlar = 0;
  }
  mpu_region(0, loader_flash, ap_ro, false);
  mpu_region(3, shared_ram, ap_rw, true);
  mpu_region(4, game_stack, ap_rw, true);
  mpu_region(5, game_ram, ap_rw, false);

  // faults go to their own handlers (sandbox_gate.S), not HardFault
  scb_hw->shcsr |= M33_SHCSR_MEMFAULTENA_BITS | M33_SHCSR_BUSFAULTENA_BITS | M33_SHCSR_USGFAULTENA_BITS;
  // a requested stop runs after the interrupt that asked for it
  exception_set_priority(PENDSV_EXCEPTION, PICO_LOWEST_EXCEPTION_PRIORITY);

  // the loader overflowing its part of the stack faults instead of writing the game's
  __asm volatile("msr msplim, %0" :: "r"(game_stack.end));
}

void sandbox_start_game(uint32_t image_start, uint32_t image_end, bool trusted) {
  game.active = true;
  game.trusted = trusted;
  game.image = {image_start & ~31u, (image_end + 31) & ~31u};
  stop_reason = nullptr;
  for(auto &f : open_files)
    f = nullptr;
  sandbox_thread_control = trusted ? 2 : 3;
  // the channels were reset by launch_pre_init, so no callback of an earlier game is left
  blit::block_wave_buffer_callbacks = !trusted;

  mpu_region(1, game.image, ap_ro, false);
  mpu_region(2, {}, ap_ro, true);
}

void sandbox_set_installed_copy(uint32_t start, uint32_t end) {
  game.installed = {start & ~31u, (end + 31) & ~31u};
  // two regions must not overlap (the address would fault)
  if(game.installed.start < game.image.end && game.image.start < game.installed.end)
    game.installed = {};
  mpu_region(2, game.installed, ap_ro, true);
}

void sandbox_stop_game() {
  // block_wave_buffer_callbacks stays until the next game: the channels still hold this one's
  game = {};
  stop_reason = nullptr;
}

bool sandbox_game_running() {
  return game.active;
}

bool sandbox_in_call() {
  return in_call;
}

const char *sandbox_stop_reason() {
  return stop_reason;
}

void sandbox_abort_game(const char *reason) {
  if(!stop_reason)
    stop_reason = reason;
  if(in_call)
    scb_hw->icsr = M33_ICSR_PENDSVSET_BITS;
}

uint32_t sandbox_call(const void *function, uint32_t arg0, uint32_t arg1) {
  // one call at a time: the game has one stack
  if(!game.active || stop_reason || in_call || !function)
    return 0;

  in_call = true;
  __asm volatile("msr psplim, %0" :: "r"(game_stack.start));
  if(!game.trusted) {
    mpu_hw->ctrl = M33_MPU_CTRL_PRIVDEFENA_BITS | M33_MPU_CTRL_ENABLE_BITS;
    __dsb();
    __isb();
  }

  auto result = sandbox_enter(function, arg0, arg1, game_stack.end);

  mpu_hw->ctrl = 0;
  __dsb();
  __isb();
  in_call = false;
  in_api = false;
  return result;
}

void sandbox_set_entry_points(const void *tick, const void *render) {
  game.tick = tick;
  game.render = render;
}

int sandbox_tick(uint32_t time) {
  return int(sandbox_call(game.tick, time));
}

void sandbox_render(uint32_t time) {
  sandbox_call(game.render, time);
}

void sandbox_message_received(const uint8_t *data, uint16_t len) {
  auto callback = blit::api_data.message_received;
  if(!game.active || !callback)
    return;

  if(!game.trusted) {
    // the game can't read the loader's buffer
    if(len > sizeof(shared.message))
      return;
    memcpy(shared.message, data, len);
    data = shared.message;
  }
  sandbox_call(reinterpret_cast<const void *>(callback), uintptr_t(data), len);
}

#endif
