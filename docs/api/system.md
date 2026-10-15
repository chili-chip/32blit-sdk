# System

`engine/system.hpp`

ChiliChip OS adds these calls so games can show the battery, react to USB, and read the player's settings. They were added in the API versions shown. In desktop builds, and on other hardware without them, they report "not available" (`-1`, `0` or `false`), so always handle that case. The [System Info example](../examples/index.md#system-info) shows them all.

## Battery and USB

**API 0.3+**

```cpp
int get_battery_level();
uint16_t get_battery_voltage();
bool is_usb_connected();
```

`get_battery_level()`
:   The battery charge in percent, 0–100, or `-1` if the device can't measure it. On the VGC Zero the reading is smoothed, and the percentage curve is still approximate.

`get_battery_voltage()`
:   The battery voltage in millivolts, or `0` if the device can't measure it.

`is_usb_connected()`
:   `true` while a computer has the console connected over USB. A charger alone doesn't count.

```cpp
int level = get_battery_level();
if(level >= 0 && level < 10)
  screen.text("LOW BATTERY", minimal_font, Point(2, 2));
```

## Settings

**API 0.3+**

```cpp
struct SystemSettings {
  uint8_t brightness = 15;      // display brightness, 0 (dimmest) to 15
  uint8_t volume = 100;         // master volume, 0 to 100 percent
  uint16_t sleep_timeout = 0;   // seconds without input before the screen sleeps, 0 for never
  uint8_t reserved[12] = {};
};

bool get_system_settings(SystemSettings &settings);
bool set_system_settings(const SystemSettings &settings);
```

`get_system_settings(settings)`
:   Fills in the player's current settings. Returns `false` if the device has none.

`set_system_settings(settings)`
:   Applies and saves new settings. It's meant for the home screen and settings apps; games should leave the player's settings alone.

The firmware applies brightness, volume and screen sleep to every game itself, so games only read these to show them.

## System info

**API 0.4+**

```cpp
struct SystemInfo {
  char firmware_version[24];   // version of the loader
  char serial[24];             // unique device id, as hex
  uint32_t app_flash_total;    // bytes of flash for installed games
  uint32_t app_flash_used;     // bytes of that in use
  uint32_t storage_total;      // size of the USB drive in bytes
  uint32_t storage_free;       // free space on the USB drive in bytes
};

bool get_system_info(SystemInfo &info);
```

Fills in firmware and storage details, for an "about" or storage screen. Returns `false` if the device doesn't provide them.

## Checking support

On a device that doesn't provide a call, it returns its "not available" value, so one check covers the desktop and other hardware:

```cpp
SystemSettings settings;
if(get_system_settings(settings)) {
  // show settings.volume
} else {
  // older firmware, or the desktop
}
```

On the VGC Zero itself you don't need version checks: a `.blit` only installs on a loader with at least the API version of the SDK it was built with, so every call here is there. See [Publishing a game](../guides/publishing.md#compatibility-with-players-consoles).
