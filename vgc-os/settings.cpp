#include <algorithm>
#include <cstdio>
#include <cstring>

#include "settings.hpp"
#include "games.hpp"
#include "ui.hpp"

using namespace blit;

enum SettingsItem {
  Brightness,
  Volume,
  Sleep,
  Games,
  Storage,
  About,
  ItemCount
};

static const uint16_t sleep_options[] = {0, 60, 120, 300, 600};
static constexpr int num_sleep_options = sizeof(sleep_options) / sizeof(sleep_options[0]);
static constexpr int volume_step = 10;

static int selected = 0;
static int selected_game = 0;

static SystemSettings settings;
static SystemInfo info;
static bool have_info = false;

static int sleep_index() {
  for(int i = 0; i < num_sleep_options; i++)
    if(sleep_options[i] >= settings.sleep_timeout)
      return i;
  return num_sleep_options - 1;
}

static std::string sleep_text(uint16_t seconds) {
  if(!seconds)
    return "OFF";
  return std::to_string(seconds / 60) + " MIN";
}

void settings_enter(Screen screen) {
  switch(screen) {
    case Screen::settings:
      // keeps the defaults when the device has no settings (desktop)
      get_system_settings(settings);
      break;

    case Screen::games:
      selected_game = std::min(selected_game, std::max(0, int(installed_software().size()) - 1));
      break;

    case Screen::storage:
    case Screen::about:
      have_info = get_system_info(info);
      break;

    default:
      break;
  }
}

static void update_settings(uint32_t pressed) {
  if(pressed & Button::DPAD_UP)
    selected = (selected + ItemCount - 1) % ItemCount;
  if(pressed & Button::DPAD_DOWN)
    selected = (selected + 1) % ItemCount;

  int adjust = 0;
  if(pressed & Button::DPAD_LEFT)
    adjust = -1;
  if(pressed & Button::DPAD_RIGHT)
    adjust = 1;

  if(adjust) {
    auto old = settings;

    if(selected == Brightness)
      settings.brightness = std::clamp(settings.brightness + adjust, 0, 15);
    else if(selected == Volume)
      settings.volume = std::clamp(settings.volume + adjust * volume_step, 0, 100);
    else if(selected == Sleep)
      settings.sleep_timeout = sleep_options[std::clamp(sleep_index() + adjust, 0, num_sleep_options - 1)];

    if(memcmp(&old, &settings, sizeof(settings)) != 0)
      set_system_settings(settings);
  }

  if(pressed & Button::A) {
    if(selected == Games)
      go_to(Screen::games);
    else if(selected == Storage)
      go_to(Screen::storage);
    else if(selected == About)
      go_to(Screen::about);
  }

  if(pressed & (Button::B | Button::MENU))
    go_to(Screen::home);
}

static void update_games(uint32_t pressed) {
  auto software = installed_software();
  int count = software.size();

  if(count) {
    if(pressed & Button::DPAD_UP)
      selected_game = (selected_game + count - 1) % count;
    if(pressed & Button::DPAD_DOWN)
      selected_game = (selected_game + 1) % count;
    if(pressed & Button::A)
      open_info(software[selected_game], Screen::games);
  }

  if(pressed & Button::B)
    go_to(Screen::settings);
}

void settings_update(Screen screen, uint32_t pressed) {
  switch(screen) {
    case Screen::settings:
      update_settings(pressed);
      break;

    case Screen::games:
      update_games(pressed);
      break;

    case Screen::storage:
    case Screen::about:
      if(pressed & Button::B)
        go_to(Screen::settings);
      break;

    default:
      break;
  }
}

static void render_settings() {
  std::vector<ListRow> rows(ItemCount);

  rows[Brightness].label = "Brightness";
  rows[Brightness].bar = settings.brightness;
  rows[Brightness].bar_max = 15;

  rows[Volume].label = "Volume";
  rows[Volume].bar = settings.volume;

  rows[Sleep].label = "Sleep after";
  rows[Sleep].value = sleep_text(settings.sleep_timeout);

  rows[Games].label = "Installed software";
  rows[Games].value = std::to_string(installed_software().size());

  rows[Storage].label = "Storage";
  rows[About].label = "About";

  // rows that open a page say so, the others change with left and right
  bool opens = selected == Games || selected == Storage || selected == About;
  draw_list("SETTINGS", rows, selected, opens ? "A OPEN" : "< > CHANGE", "B BACK");
}

static void render_games() {
  std::vector<ListRow> rows;

  // games, the files apps play (Bitsy games) and the apps themselves (Bitsy)
  for(int index : installed_software()) {
    auto &game = get_games()[index];
    rows.push_back({game.title, format_size(game.size)});
  }

  bool empty = rows.empty();
  if(empty) {
    rows.push_back({"Nothing installed"});
    rows.back().dim = true;
  }

  draw_list("SOFTWARE", rows, empty ? -1 : selected_game, empty ? "" : "A INFO", "B BACK");
}

// "0.1 OF 3.6 MB", short enough to sit next to its label
static std::string usage_text(uint32_t used, uint32_t total) {
  const uint32_t mb = 1024 * 1024;
  if(total < mb)
    return format_size(used) + " OF " + format_size(total);

  auto tenths = [mb](uint32_t bytes) {
    uint32_t t = uint32_t((uint64_t(bytes) * 10 + mb / 2) / mb);
    return std::to_string(t / 10) + "." + std::to_string(t % 10);
  };
  return tenths(used) + " OF " + tenths(total) + " MB";
}

// a label, the usage on the right and a bar under them
static void draw_usage(int y, const char *label, uint32_t used, uint32_t total) {
  const int w = screen.bounds.w;
  auto usage = usage_text(used, total);

  screen.pen = theme::text;
  screen.text(label, minimal_font, Point(margin, y));
  screen.pen = theme::muted;
  screen.text(usage, minimal_font, Point(w - margin - text_width(usage), y));

  // in kB, so the product fits in an int
  draw_bar(Rect(margin, y + 11, w - margin * 2, 3), used / 1024, std::max<uint32_t>(1, total / 1024));
}

static void render_storage() {
  draw_background();
  draw_status_bar("STORAGE");

  if(!have_info) {
    screen.pen = theme::slate;
    screen.text("Not available", minimal_font, Point(margin, 20));
  } else {
    draw_usage(20, "Games", info.app_flash_used, info.app_flash_total);
    draw_usage(44, "USB drive", info.storage_total - info.storage_free, info.storage_total);
  }

  draw_hints("", "B BACK");
}

static void render_about() {
  const int w = screen.bounds.w;

  draw_background();
  draw_status_bar("ABOUT");

  int y = 18;
  auto add = [&y, w](const char *label, const std::string &value) {
    screen.pen = theme::muted;
    screen.text(label, minimal_font, Point(margin, y));
    auto fitted = fit_text(value, w - margin * 3 - text_width(label));
    screen.pen = theme::text;
    screen.text(fitted, minimal_font, Point(w - margin - text_width(fitted), y));
    y += 12;
  };

  add("OS version", get_metadata().version);

  if(have_info) {
    add("Firmware", info.firmware_version);

    int level = get_battery_level();
    if(level >= 0) {
      std::string battery = std::to_string(level) + "%";
      if(int mv = get_battery_voltage()) {
        char volts[16];
        snprintf(volts, sizeof(volts), "  %i.%02i V", mv / 1000, mv % 1000 / 10);
        battery += volts;
      }
      add("Battery", battery);
    }

    add("Serial", info.serial);
  }

  draw_hints("", "B BACK");
}

void settings_render(Screen screen) {
  switch(screen) {
    case Screen::settings:
      render_settings();
      break;
    case Screen::games:
      render_games();
      break;
    case Screen::storage:
      render_storage();
      break;
    case Screen::about:
      render_about();
      break;
    default:
      break;
  }
}
