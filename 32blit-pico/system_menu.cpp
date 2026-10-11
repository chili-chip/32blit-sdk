#include <algorithm>
#include <cstdio>
#include <cstring>

#include "system_menu.hpp"
#include "blit_launch.hpp"
#include "display.hpp"
#include "overlay.hpp"
#include "power.hpp"
#include "settings.hpp"

#include "audio/audio.hpp"
#include "engine/api_private.hpp"
#include "engine/engine.hpp"
#include "graphics/font.hpp"

using namespace blit;

extern int (*do_tick)(uint32_t time);

enum Item {
  Resume = 0,
  Volume,
  Brightness,
  Battery,
  Quit,
  ItemCount
};

static const uint8_t volume_step = 10;

static bool menu_open = false;
static bool resume_pending = false; // resume once all buttons are released
static bool quit_pending = false;   // quit once all buttons are released, so the launcher doesn't see the press
static int (*game_tick)(uint32_t time) = nullptr;
static void (*game_render)(uint32_t time) = nullptr;

static int selected = Resume;
static uint32_t last_buttons = 0;
static int dim_pages = 0; // framebuffer pages still showing the undimmed game

static bool can_open() {
  if(do_tick == blit::tick || is_render_overlay_enabled())
    return false;

  auto meta = get_running_game_metadata();
  if(!meta)
    return false;

  // not over the launcher
  auto type = reinterpret_cast<const char *>(meta + 1);
  if(memcmp(type, "BLITTYPE", 8) == 0) {
    auto type_meta = reinterpret_cast<const RawTypeMetadata *>(type + 8);
    if(strcmp(type_meta->category, "launcher") == 0)
      return false;
  }

  return true;
}

static void restore_game() {
  do_tick = game_tick;
  blit::render = game_render;
  menu_open = false;
  resume_pending = false;
  quit_pending = false;

  unmute();
}

static void move_selection(int dir) {
  do {
    selected = (selected + dir + ItemCount) % ItemCount;
  } while(selected == Battery); // display only
}

static int menu_tick(uint32_t time) {
  uint32_t buttons = api_data.buttons;
  uint32_t pressed = buttons & ~last_buttons;
  last_buttons = buttons;

  if(quit_pending) {
    if(!buttons) {
      quit_pending = false;
      quit_to_launcher();
    }
    return 10;
  }

  if(resume_pending) {
    // so the game doesn't see the button that closed the menu
    if(!buttons)
      restore_game();
    return 10;
  }

  if(pressed & Button::DPAD_UP)
    move_selection(-1);
  if(pressed & Button::DPAD_DOWN)
    move_selection(1);

  int adjust = 0;
  if(pressed & Button::DPAD_LEFT)
    adjust = -1;
  else if(pressed & Button::DPAD_RIGHT)
    adjust = 1;

  if(adjust) {
    auto settings = get_current_settings();

    if(selected == Volume)
      settings.volume = std::max(0, std::min(100, settings.volume + adjust * volume_step));
    else if(selected == Brightness)
      settings.brightness = std::max(0, std::min(15, settings.brightness + adjust));

    apply_settings(settings);
    blit::volume = 0; // still paused
  }

  if(pressed & Button::A) {
    if(selected == Resume)
      resume_pending = true;
    else if(selected == Quit) {
      quit_pending = true;
      return 10;
    }
  }

  if(pressed & Button::B)
    resume_pending = true;

  return 20;
}

static void draw_bar(const Rect &r, int value, int max) {
  screen.pen = Pen(70, 70, 90);
  screen.rectangle(r);
  screen.pen = Pen(255, 255, 255);
  screen.rectangle(Rect(r.x, r.y, r.w * value / max, r.h));
}

static void menu_render(uint32_t time) {
  const int w = screen.bounds.w, h = screen.bounds.h;
  const bool small = h < 100; // lores

  // dim the paused game once per page, it doesn't redraw underneath
  if(dim_pages > 0) {
    screen.pen = Pen(0, 0, 0, 160);
    screen.rectangle(Rect(0, 0, w, h));
    dim_pages--;
  }

  static const char *const labels[ItemCount] = {"Resume", "Volume", "Brightness", "Battery", "Quit to home"};
  static const char *const short_labels[ItemCount] = {"Resume", "Vol", "Light", "Batt", "Quit"};

  const int margin = small ? 2 : 6;
  const int row_h = small ? 10 : 15;
  const int title_h = small ? 0 : 16;
  const int bar_w = small ? 22 : 40;

  Rect panel(margin, 0, w - margin * 2, title_h + row_h * ItemCount + 4);
  panel.y = (h - panel.h) / 2;

  screen.pen = Pen(90, 90, 120);
  screen.rectangle(panel);
  screen.pen = Pen(16, 16, 24);
  screen.rectangle(Rect(panel.x + 1, panel.y + 1, panel.w - 2, panel.h - 2));

  if(title_h) {
    screen.pen = Pen(160, 160, 190);
    screen.text("Paused", minimal_font, Rect(panel.x, panel.y + 2, panel.w, title_h), true, TextAlign::center_center);
  }

  auto &settings = get_current_settings();

  for(int i = 0; i < ItemCount; i++) {
    Rect row(panel.x + 2, panel.y + 2 + title_h + i * row_h, panel.w - 4, row_h);

    if(i == selected) {
      screen.pen = Pen(50, 80, 150);
      screen.rectangle(row);
    }

    Rect inner(row.x + 3, row.y, row.w - 6, row.h);
    screen.pen = Pen(255, 255, 255);
    screen.text(small ? short_labels[i] : labels[i], minimal_font, inner, true, TextAlign::center_left);

    Rect bar(inner.x + inner.w - bar_w, row.y + row_h / 2 - 2, bar_w, 4);

    if(i == Volume)
      draw_bar(bar, settings.volume, 100);
    else if(i == Brightness)
      draw_bar(bar, settings.brightness, 15);
    else if(i == Battery) {
      char buf[16];
      int level = ::get_battery_level();

      if(level < 0)
        snprintf(buf, sizeof(buf), "--");
      else if(small || !::is_usb_connected())
        snprintf(buf, sizeof(buf), "%i%%", level);
      else
        snprintf(buf, sizeof(buf), "%i%% USB", level);

      screen.text(buf, minimal_font, inner, true, TextAlign::center_right);
    }
  }
}

void system_menu_toggle() {
  if(menu_open) {
    resume_pending = true;
    return;
  }

  if(!can_open())
    return;

  game_tick = do_tick;
  game_render = blit::render;

  do_tick = menu_tick;
  blit::render = menu_render;

  menu_open = true;
  resume_pending = false;
  selected = Resume;
  last_buttons = api_data.buttons;

  // show the same frame on both pages, or the paused game flickers between two
  dim_pages = 1;
  if(fb_double_buffer) {
    auto page0 = reinterpret_cast<uint8_t *>(screen_fb), page1 = page0 + get_display_page_size();
    auto other = screen.data == page0 ? page1 : page0;
    memcpy(other, screen.data, screen.bounds.area() * pixel_format_stride[int(screen.format)]);
    dim_pages = 2;
  }

  // the game is paused, so is its sound
  blit::volume = 0;
}

bool system_menu_is_open() {
  return menu_open;
}

void system_menu_reset() {
  if(menu_open)
    restore_game();
}
