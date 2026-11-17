#pragma once
#include <cstdint>

// Finds the optional parts of the console at start-up (32blit-sdk #90).
// Call after init_i2c().
void init_modules();

// blit::HardwareFlags for the parts found
uint32_t get_hardware_flags();
