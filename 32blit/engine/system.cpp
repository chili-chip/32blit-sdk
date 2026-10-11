#include "system.hpp"
#include "api_private.hpp"

namespace blit {
  int get_battery_level() {
    return api.get_battery_level ? api.get_battery_level() : -1;
  }

  uint16_t get_battery_voltage() {
    return api.get_battery_voltage ? api.get_battery_voltage() : 0;
  }

  bool is_usb_connected() {
    return api.is_usb_connected ? api.is_usb_connected() : false;
  }

  bool get_system_settings(SystemSettings &settings) {
    return api.get_system_settings ? api.get_system_settings(settings) : false;
  }

  bool set_system_settings(const SystemSettings &settings) {
    return api.set_system_settings ? api.set_system_settings(settings) : false;
  }

  bool get_system_info(SystemInfo &info) {
    return api.get_system_info ? api.get_system_info(info) : false;
  }
}
