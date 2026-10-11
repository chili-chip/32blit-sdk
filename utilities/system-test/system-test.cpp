// Shows the system services from API 0.3: battery, USB and settings.
// Press A for a test tone (to hear the volume setting), HOME for the system menu.
#include <cstdio>

#include "32blit.hpp"

using namespace blit;

void init() {
  set_screen_mode(ScreenMode::hires);
}

static void row(int y, const char *label, const char *value) {
  screen.pen = Pen(160, 160, 190);
  screen.text(label, minimal_font, Point(4, y));
  screen.pen = Pen(255, 255, 255);
  screen.text(value, minimal_font, Point(screen.bounds.w - 4, y), true, TextAlign::top_right);
}

void render(uint32_t time) {
  char buf[32];

  screen.pen = Pen(0, 0, 0);
  screen.clear();

  screen.pen = Pen(255, 255, 255);
  screen.text("System test", minimal_font, Point(screen.bounds.w / 2, 4), true, TextAlign::top_center);

  int level = get_battery_level();
  if(level < 0)
    snprintf(buf, sizeof(buf), "not measured");
  else
    snprintf(buf, sizeof(buf), "%i%%", level);
  row(20, "Battery", buf);

  uint16_t mv = get_battery_voltage();
  snprintf(buf, sizeof(buf), "%i.%02i V", mv / 1000, mv % 1000 / 10);
  row(32, "Voltage", buf);

  row(44, "USB", is_usb_connected() ? "connected" : "not connected");

  SystemSettings settings;
  if(get_system_settings(settings)) {
    snprintf(buf, sizeof(buf), "%i / 15", settings.brightness);
    row(60, "Brightness", buf);

    snprintf(buf, sizeof(buf), "%i%%", settings.volume);
    row(72, "Volume", buf);

    if(settings.sleep_timeout)
      snprintf(buf, sizeof(buf), "%i s", settings.sleep_timeout);
    else
      snprintf(buf, sizeof(buf), "never");
    row(84, "Sleep", buf);
  } else
    row(60, "Settings", "not available");

  screen.pen = Pen(120, 120, 140);
  screen.text("A: test tone", minimal_font, Point(4, screen.bounds.h - 24));
  screen.text("HOME: menu, hold: quit", minimal_font, Point(4, screen.bounds.h - 12));
}

void update(uint32_t time) {
  if(buttons.pressed & Button::A) {
    channels[0].waveforms = Waveform::SQUARE;
    channels[0].frequency = 440;
    channels[0].attack_ms = 5;
    channels[0].decay_ms = 100;
    channels[0].sustain = 0;
    channels[0].release_ms = 50;
    channels[0].trigger_attack();
  }
}
