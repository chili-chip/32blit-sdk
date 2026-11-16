#pragma once
// Sandbox for .blit games on RP2350 (32blit-sdk #91). See sandbox.cpp.
#include <cstdint>

#if defined(BLIT_SANDBOX) && defined(BUILD_LOADER)
#define BLIT_SANDBOX_ENABLED 1
#else
#define BLIT_SANDBOX_ENABLED 0
#endif

#if BLIT_SANDBOX_ENABLED
// Once at boot, before any game runs.
void sandbox_init();

// A game is about to start: [image_start, image_end) is its flash.
// A trusted game (the home launcher) runs privileged, only on its own stack.
void sandbox_start_game(uint32_t image_start, uint32_t image_end, bool trusted);
// The installed copy of an address-translated game: readable (its metadata is there).
void sandbox_set_installed_copy(uint32_t start, uint32_t end);
void sandbox_set_entry_points(const void *tick, const void *render);
void sandbox_stop_game();
bool sandbox_game_running();

// Runs game code: unprivileged behind the MPU, with the API reached through
// svc. Returns the function's r0, or 0 if the game was stopped.
uint32_t sandbox_call(const void *function, uint32_t arg0 = 0, uint32_t arg1 = 0);
bool sandbox_in_call();

// The game's tick, render and multiplayer callback, through sandbox_call.
int sandbox_tick(uint32_t time);
void sandbox_render(uint32_t time);
void sandbox_message_received(const uint8_t *data, uint16_t len);

// Stops the game: at once if it is running (from an interrupt), else before its next call.
void sandbox_abort_game(const char *reason);

// Why the game was stopped, or nullptr while it may run.
const char *sandbox_stop_reason();
#endif
