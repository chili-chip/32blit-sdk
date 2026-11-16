// Runs sandbox.cpp and sandbox_gate.S on an emulated Cortex-M33 (QEMU
// mps2-an505), with game.cpp as the game. Prints each check over semihosting
// and exits non-zero if one fails. See README.md here.
#include <cstdarg>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>

#include "hardware/structs/scb.h"

#include "audio/audio.hpp"
#include "engine/api_private.hpp"
#include "sandbox.hpp"

// semihosting (Arm "Semihosting for AArch32 and AArch64", SYS_WRITE0 and SYS_EXIT_EXTENDED)
static int semihost(int op, const void *arg) {
  register int r0 asm("r0") = op;
  register const void *r1 asm("r1") = arg;
  __asm volatile("bkpt 0xab" : "+r"(r0) : "r"(r1) : "memory");
  return r0;
}

static void print(const char *s) {
  semihost(0x04, s);
}

[[noreturn]] static void quit(int code) {
  uint32_t block[2] = {0x20026, uint32_t(code)}; // ADP_Stopped_ApplicationExit
  semihost(0x20, block);
  while(true) {}
}

// stand-ins for the pico-sdk and the loader
extern "C" [[noreturn]] void panic(const char *fmt, ...) {
  print("panic: ");
  print(fmt);
  print("\n");
  quit(2);
}

extern "C" bool exception_set_priority(unsigned num, uint8_t priority) {
  reinterpret_cast<volatile uint8_t *>(0xE000ED14)[num] = priority; // SHPR1-3 start at exception 4
  return true;
}

namespace blit { volatile bool block_wave_buffer_callbacks = false; }

[[gnu::section(".bss.sandbox_shared.channels")]]
alignas(blit::AudioChannel) uint8_t sandbox_channel_storage[sizeof(blit::AudioChannel) * 8];

// the loader's own state a game must not reach
static volatile uint32_t loader_secret = 1234;
static char loader_text[] = "the loader's own string"; // in RAM: loader flash is readable by design
static bool erased = false, launched = false;
static char debug_seen[64];
static int fake_file;

static uint32_t now() { return 42; }
static uint32_t random() { return 4; }
static void exit_game(bool) {}
static void debug(const char *message) { strncpy(debug_seen, message, sizeof(debug_seen) - 1); }
static void *open_file(const std::string &name, int) { return name == "save.dat" ? &fake_file : nullptr; }
static int32_t read_file(void *, uint32_t, uint32_t length, char *buffer) { memcpy(buffer, "DATA", length < 4 ? length : 4); return 4; }
static int32_t close_file(void *) { return 0; }
static bool launch(const char *) { return launched = true; }
static void erase_game(uint32_t) { erased = true; }

static uint32_t unused() { return 0; }

static blit::APIConst make_api() {
  blit::APIConst a{};
  // the slots the ChiliChip loader fills (main.cpp); sandbox_init checks the game's table matches
  auto slots = reinterpret_cast<const void **>(reinterpret_cast<char *>(&a) + 8);
  for(int i = 0; i < 50; i++) {
    bool empty = i == 19 || i == 22 || i == 23 || i == 31 || i == 32 || (i >= 35 && i <= 39);
    slots[i] = empty ? nullptr : reinterpret_cast<const void *>(unused);
  }
  a.version_major = blit::api_version_major;
  a.version_minor = blit::api_version_minor;
  a.channels = reinterpret_cast<blit::AudioChannel *>(sandbox_channel_storage);
  a.now = now;
  a.random = random;
  a.exit = exit_game;
  a.debug = debug;
  a.open_file = open_file;
  a.read_file = read_file;
  a.close_file = close_file;
  a.launch = launch;
  a.erase_game = erase_game;
  return a;
}

// the game sees sandbox.cpp's table; these are the slots it has
static const blit::APIConst real_api = make_api();
[[gnu::section(".bss.api_data")]] static blit::APIData real_api_data;

namespace blit {
  const APIConst &api = real_api;
  APIData &api_data = real_api_data;
}

// game.cpp
extern "C" {
  uint32_t game_add_seven(uint32_t), game_now_plus_one(uint32_t), game_write(uint32_t), game_read(uint32_t);
  uint32_t game_svc_return(uint32_t), game_debug(uint32_t), game_erase(uint32_t), game_launch(uint32_t);
  uint32_t game_loop(uint32_t), game_recurse(uint32_t), game_files(uint32_t), game_read_into(uint32_t);
  extern uint8_t game_buffer[256];
  extern char __game_image_start[], __game_image_end[];
}

static int failures = 0;

static void check(const char *name, bool ok) {
  print(ok ? "ok   " : "FAIL ");
  print(name);
  print("\n");
  if(!ok)
    failures++;
}

static bool stopped_for(const char *words) {
  auto reason = sandbox_stop_reason();
  return reason && strstr(reason, words);
}

static void restart(bool trusted = false) {
  sandbox_stop_game();
  sandbox_start_game(uintptr_t(__game_image_start), uintptr_t(__game_image_end), trusted);
}

static uint32_t call(uint32_t (*fn)(uint32_t), uint32_t arg = 0) {
  return sandbox_call(reinterpret_cast<const void *>(fn), arg);
}

extern "C" void isr_systick() {
  *reinterpret_cast<volatile uint32_t *>(0xE000E010) = 0; // SysTick off
  sandbox_abort_game("test timeout");
}

int main() {
  sandbox_init();
  restart();

  check("a game call returns its result", call(game_add_seven, 1) == 8 && !sandbox_stop_reason());
  check("the API works through svc", call(game_now_plus_one) == 43);
  check("the audio callbacks are blocked for a sandboxed game", blit::block_wave_buffer_callbacks);

  check("writing the loader's RAM stops the game",
        call(game_write, uintptr_t(&loader_secret)) == 0 && stopped_for("not its own") && loader_secret == 1234);
  check("a stopped game is not called again", call(game_add_seven, 1) == 0);

  restart();
  check("writing its own RAM is fine", call(game_write, uintptr_t(game_buffer)) == 1 && !sandbox_stop_reason());

  restart();
  call(game_read, 0xE000ED00); // SCB CPUID
  check("reading the system control block stops the game", sandbox_stop_reason() != nullptr);

  restart();
  check("svc 255 outside the gate gives no privilege",
        call(game_svc_return, uintptr_t(&loader_secret)) == 0 && stopped_for("outside the gate") && loader_secret == 1234);

  restart();
  check("a loader pointer passed to the API stops the game",
        call(game_debug, uintptr_t(loader_text)) == 0 && stopped_for("not its own") && !debug_seen[0]);

  restart();
  strcpy(reinterpret_cast<char *>(game_buffer), "hello");
  check("a game string passed to the API is fine",
        call(game_debug, uintptr_t(game_buffer)) == 1 && !strcmp(debug_seen, "hello"));

  restart();
  check("erase_game stops the game", call(game_erase) == 0 && stopped_for("erase") && !erased);

  restart();
  check("launch is refused, the game goes on", call(game_launch) == 5 && !launched && !sandbox_stop_reason());

  restart();
  *reinterpret_cast<volatile uint32_t *>(0xE000E014) = 2000000; // SysTick reload
  *reinterpret_cast<volatile uint32_t *>(0xE000E018) = 0;
  *reinterpret_cast<volatile uint32_t *>(0xE000E010) = 7; // processor clock, interrupt, enable
  check("a game that never returns is stopped from an interrupt", call(game_loop) == 0 && stopped_for("test timeout"));

  restart();
  check("a stack overflow stops the game", call(game_recurse, 1000) == 0 && stopped_for("stack overflow"));

  restart();
  memset(game_buffer, 0, 8);
  check("files: open, read into game RAM, close",
        call(game_files, 0) == 4 && !memcmp(game_buffer, "DATA", 4) && !sandbox_stop_reason());

  restart();
  check("a made-up file handle stops the game", call(game_files, uintptr_t(&fake_file) + 64) == 0 && stopped_for("not its own"));

  restart();
  check("reading a file into the loader's RAM stops the game",
        call(game_read_into, uintptr_t(&loader_secret)) == 0 && stopped_for("not its own") && loader_secret == 1234);

  restart(true);
  check("the trusted launcher is not blocked from audio callbacks", !blit::block_wave_buffer_callbacks);
  check("the trusted launcher runs privileged (the risk signing must cover)",
        call(game_write, uintptr_t(&loader_secret)) == 1 && loader_secret == 0xBAD);
  loader_secret = 1234;

  restart();
  check("after all that the sandbox still works", call(game_now_plus_one) == 43 && loader_secret == 1234);

  print(failures ? "sandbox test: FAILED\n" : "sandbox test: all passed\n");
  quit(failures ? 1 : 0);
}

// start-up: no pico-sdk runtime here
extern "C" {
  extern char __StackTop[], __data_start__[], __data_end__[], __data_source__[], __bss_start__[], __bss_end__[];
  extern void (*__init_array_start[])(), (*__init_array_end[])();
  void isr_hardfault(), isr_memmanage(), isr_busfault(), isr_usagefault(), isr_svcall(), isr_pendsv();

  [[noreturn]] void reset_handler() {
    memcpy(__data_start__, __data_source__, __data_end__ - __data_start__);
    memset(__bss_start__, 0, __bss_end__ - __bss_start__);
    scb_hw->cpacr |= 0xF << 20; // FPU
    __asm volatile("dsb; isb");
    for(auto f = __init_array_start; f != __init_array_end; f++)
      (*f)();
    quit(main());
  }

  static void unexpected() {
    print("unexpected exception\n");
    quit(3);
  }

  [[gnu::section(".vectors"), gnu::used]]
  static const void *const vectors[16] = {
    __StackTop, reinterpret_cast<const void *>(reset_handler), reinterpret_cast<const void *>(unexpected),
    reinterpret_cast<const void *>(isr_hardfault), reinterpret_cast<const void *>(isr_memmanage),
    reinterpret_cast<const void *>(isr_busfault), reinterpret_cast<const void *>(isr_usagefault),
    reinterpret_cast<const void *>(unexpected), nullptr, nullptr, nullptr,
    reinterpret_cast<const void *>(isr_svcall), nullptr, nullptr,
    reinterpret_cast<const void *>(isr_pendsv), reinterpret_cast<const void *>(isr_systick),
  };
}
