#include <algorithm>
#include <cstdio>

#include "ui.hpp"

using namespace blit;

uint32_t get_pressed(uint32_t time) {
  static uint32_t held_since = 0, last_repeat = 0;
  const uint32_t dpad = Button::DPAD_UP | Button::DPAD_DOWN | Button::DPAD_LEFT | Button::DPAD_RIGHT;

  uint32_t pressed = buttons.pressed;

  if(pressed & dpad)
    held_since = last_repeat = time;
  else if(buttons.state & dpad) {
    if(time - held_since > 400 && time - last_repeat > 110) {
      pressed |= buttons.state & dpad;
      last_repeat = time;
    }
  }

  return pressed;
}

std::string format_size(uint32_t bytes) {
  char buf[16];
  if(bytes >= 1024 * 1024)
    snprintf(buf, sizeof(buf), "%u.%u MB", unsigned(bytes / (1024 * 1024)), unsigned(bytes % (1024 * 1024) * 10 / (1024 * 1024)));
  else
    snprintf(buf, sizeof(buf), "%u KB", unsigned((bytes + 1023) / 1024));
  return buf;
}
