#pragma once
#include <cstdint>

// Dims, then blanks, the screen after the "screen sleep" setting's idle time.
// Any button wakes it again.

// call after update_input. While the screen is off, the button press that
// wakes it is hidden from the game
void update_screen_sleep(uint32_t time);

// set the display brightness from the settings, dimmed if the screen is idle
void apply_screen_brightness();

bool is_screen_asleep();
