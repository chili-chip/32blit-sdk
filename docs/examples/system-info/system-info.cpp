// System info: battery, USB, the player's settings and firmware details.
//
// These calls come from the console's firmware (the loader). On desktop
// builds, and on older firmware, they report "not available" instead.
#include <cstdio>

#include "32blit.hpp"

using namespace blit;

std::string kb(uint32_t bytes) {
  return std::to_string(bytes / 1024) + " KB";
}

void init() {
  set_screen_mode(ScreenMode::hires);
}

void update(uint32_t time) {
}

void render(uint32_t time) {
  screen.pen = Pen(20, 20, 30);
  screen.clear();

  std::string lines;

  // this game's own metadata.yml, and the SDK it was built with
  auto meta = get_metadata();
  lines += std::string(meta.title) + " " + meta.version + "\n";
  lines += std::string("SDK ") + get_version_string() + "\n\n";

  // battery: -1 means the device can't measure it
  int level = get_battery_level();
  if(level >= 0) {
    char buf[32];
    snprintf(buf, sizeof(buf), "Battery %d%% (%.2f V)", level, get_battery_voltage() / 1000.0);
    lines += std::string(buf) + "\n";
  } else {
    lines += "Battery: n/a\n";
  }
  lines += std::string("USB: ") + (is_usb_connected() ? "connected" : "not connected") + "\n";

  // brightness, volume and sleep are applied by the firmware; games can read them
  SystemSettings settings;
  if(get_system_settings(settings)) {
    lines += "Brightness " + std::to_string(settings.brightness) + "/15\n";
    lines += "Volume " + std::to_string(settings.volume) + "%\n";
    lines += "Sleep " + (settings.sleep_timeout ? std::to_string(settings.sleep_timeout / 60) + " min" : std::string("off")) + "\n";
  } else {
    lines += "Settings: n/a\n";
  }

  SystemInfo info;
  if(get_system_info(info)) {
    lines += std::string("FW ") + info.firmware_version + "\n";
    lines += "Games " + kb(info.app_flash_used) + "/" + kb(info.app_flash_total) + "\n";
    lines += "Drive free " + kb(info.storage_free) + "\n";
  } else {
    lines += "System info: n/a\n";
  }

  screen.pen = Pen(255, 255, 255);
  screen.text(lines, minimal_font, Point(4, 4));
}
