#include "modules.hpp"
#include "config.h"
#include "display.hpp"

#include "hardware/gpio.h"
#include "hardware/i2c.h"

#include "engine/system.hpp"

static uint32_t hardware_flags = 0;

#ifdef UTILITY_MODULE_I2C
// a part answers its address on the utility module's I2C bus. Without the
// module there are no pull-ups on the bus, but the RP2350's own keep it high.
static bool i2c_found(uint8_t address) {
  uint8_t data;
  return i2c_read_timeout_us(i2c_default, address, &data, 1, false, 2000) == 1;
}
#endif

void init_modules() {
  hardware_flags = 0;

  if(display_brightness_supported())
    hardware_flags |= blit::HW_BRIGHTNESS;

#ifdef UTILITY_MODULE_I2C
  // RV-3028-C7, LSM6DS3TR-C and DRV2605L on the utility module
  if(i2c_found(0x52))
    hardware_flags |= blit::HW_CLOCK;
  if(i2c_found(0x6A))
    hardware_flags |= blit::HW_MOTION;
  if(i2c_found(0x5A))
    hardware_flags |= blit::HW_VIBRATION;
#endif

#ifdef RADIO_POWER_PIN
  // radio off until there is a driver for it (32blit-sdk #64), which then
  // also sets HW_RADIO once the chip answers
  gpio_init(RADIO_POWER_PIN);
  gpio_put(RADIO_POWER_PIN, 0);
  gpio_set_dir(RADIO_POWER_PIN, GPIO_OUT);
#endif
}

uint32_t get_hardware_flags() {
  return hardware_flags;
}
