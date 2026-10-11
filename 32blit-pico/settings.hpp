#pragma once

#include "engine/system.hpp"

// load the stored settings and apply them, call after init_fs
void init_settings();

const blit::SystemSettings &get_current_settings();

// apply without storing (used while adjusting in the system menu)
void apply_settings(const blit::SystemSettings &settings);
// restore the volume after muting, safe to call from an interrupt
void unmute();
// write the current settings to storage if they changed
void save_settings();

// blit API
bool get_system_settings(blit::SystemSettings &settings);
bool set_system_settings(const blit::SystemSettings &settings);
