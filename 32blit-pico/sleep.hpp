#pragma once
#include <cstdint>

// Sleep mode for boards with no power switch (SLEEP_BUTTON_PIN in config.h).
// Hold Start for 2 s to sleep, and for 1 s to wake. The console also sleeps
// after the idle timeout and on a low battery. See sleep.cpp.

// Call first in main(). Goes (back) to sleep if the console was asleep and
// Start wasn't held long enough. Returns only to start the console.
void sleep_check_at_boot();

// true if this boot is a wake from sleep (Start may still be held)
bool sleep_woke();

// Call once a frame. True when it's time to sleep.
bool sleep_wanted(uint32_t time);

// Turn the console's parts off and sleep. Save the game and settings first.
[[noreturn]] void sleep_enter();
