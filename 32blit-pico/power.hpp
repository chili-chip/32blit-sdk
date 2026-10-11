#pragma once
#include <cstdint>

void init_power();
void update_power(uint32_t time);

// blit API
int get_battery_level();
uint16_t get_battery_voltage();
bool is_usb_connected();
