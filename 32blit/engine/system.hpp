#pragma once
#include <cstdint>

namespace blit {
  /**
   * Settings the firmware applies to every game.
   *
   * Brightness and volume are applied by the firmware, so games don't need
   * to do anything with them. Reading them is useful for showing them, or
   * for a settings app.
   */
  struct SystemSettings {
    uint8_t brightness = 15;    ///< Display brightness, 0 (dimmest) to 15
    uint8_t volume = 100;       ///< Master volume, 0 to 100 percent
    uint16_t sleep_timeout = 0; ///< Idle seconds before the screen sleeps, 0 for never
    uint8_t reserved[12] = {};
  };

  static_assert(sizeof(SystemSettings) == 16);

  /// Battery charge in percent (0-100), or -1 if the device can't measure it
  int get_battery_level();
  /// Battery voltage in millivolts, or 0 if the device can't measure it
  uint16_t get_battery_voltage();
  /// True while the device is connected to a computer over USB
  bool is_usb_connected();

  /// Read the current system settings. Returns false if the device has none
  bool get_system_settings(SystemSettings &settings);
  /// Apply and store new system settings. Intended for launchers and settings apps
  bool set_system_settings(const SystemSettings &settings);
}
