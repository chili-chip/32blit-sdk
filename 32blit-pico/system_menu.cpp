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

#include "theme.hpp"

using namespace blit;

extern int (*do_tick)(uint32_t time);

enum Item {
  Resume = 0,
  Volume,
  Brightness,
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
  selected = (selected + dir + ItemCount) % ItemCount;
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

static std::string running_title() {
  auto meta = get_running_game_metadata();
  if(!meta)
    return "";

  std::string title(meta->title, sizeof(meta->title));
  return title.substr(0, title.find('\0'));
}

static void menu_render(uint32_t time) {
  static ScrollingText title_text;
  const int w = screen.bounds.w, h = screen.bounds.h;
  const bool small = h < 100; // lores

  // dim the paused game once per page, it doesn't redraw underneath
  if(dim_pages > 0) {
    screen.pen = theme::dim;
    screen.rectangle(Rect(0, 0, w, h));
    dim_pages--;
  }

  static const char *const labels[ItemCount] = {"Resume", "Volume", "Brightness", "Quit to home"};
  static const char *const short_labels[ItemCount] = {"Resume", "Vol", "Light", "Quit"};

  // the game's title and the battery on top, then the rows
  const int row_h = small ? 10 : 14;
  const int title_h = small ? 0 : 17;
  const int x = small ? 1 : 8;
  Rect panel(x, 0, w - x * 2 - (small ? 2 : 4), title_h + row_h * ItemCount + 3);
  panel.y = (h - panel.h - (small ? 0 : 4)) / 2;

  draw_panel(panel);

  if(!small) {
    char battery[8] = "";
    int level = ::get_battery_level();
    if(level >= 0)
      snprintf(battery, sizeof(battery), "%i%%", level);

    int battery_w = text_width(battery);
    screen.pen = level >= 0 && level <= 15 ? theme::warning : theme::muted;
    screen.text(battery, minimal_font, Point(panel.x + panel.w - 10 - battery_w, panel.y + 5));

    screen.pen = theme::muted;
    draw_scrolling_text(title_text, running_title(), Rect(panel.x + 6, panel.y + 5, panel.w - 20 - battery_w, 9));

    screen.pen = theme::line;
    screen.h_span(Point(panel.x + 1, panel.y + 16), panel.w - 2);
  }

  auto &settings = get_current_settings();

  for(int i = 0; i < ItemCount; i++) {
    ListRow row(small ? short_labels[i] : labels[i]);
    if(i == Volume)
      row.bar = settings.volume;
    else if(i == Brightness) {
      row.bar = settings.brightness;
      row.bar_max = 15;
    }

    draw_row(Rect(panel.x + 1, panel.y + title_h + 2 + i * row_h, panel.w - 2, row_h), row, i == selected);
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
