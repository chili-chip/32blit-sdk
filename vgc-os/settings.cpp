#include <algorithm>
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
    return "Off";
  return std::to_string(seconds / 60) + " min";
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

  rows[Sleep].label = "Screen sleep";
  rows[Sleep].value = sleep_text(settings.sleep_timeout);

  rows[Games].label = "Installed software";
  rows[Games].value = std::to_string(installed_software().size());

  rows[Storage].label = "Storage";
  rows[About].label = "About";

  draw_list("Settings", rows, selected);
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

  draw_list("Installed software", rows, empty ? -1 : selected_game);
}

static std::string usage_text(uint32_t used, uint32_t total) {
  return format_size(used) + " of " + format_size(total);
}

static int permille(uint32_t used, uint32_t total) {
  return total ? int(uint64_t(used) * 1000 / total) : 0;
}

static void render_storage() {
  std::vector<ListRow> rows;

  if(!have_info) {
    rows.push_back({"Not available"});
    rows.back().dim = true;
  } else {
    rows.push_back({"Games", usage_text(info.app_flash_used, info.app_flash_total)});
    ListRow bar;
    bar.bar = permille(info.app_flash_used, info.app_flash_total);
    bar.bar_max = 1000;
    rows.push_back(bar);

    uint32_t drive_used = info.storage_total - info.storage_free;
    rows.push_back({"USB drive", usage_text(drive_used, info.storage_total)});
    bar.bar = permille(drive_used, info.storage_total);
    rows.push_back(bar);
  }

  draw_list("Storage", rows, -1);
}

static void render_about() {
  std::vector<ListRow> rows;

  auto add = [&rows](const std::string &label, const std::string &value) {
    rows.push_back({label});
    rows.push_back({"  " + value});
    rows.back().dim = true;
  };

  add("OS version", get_metadata().version);

  if(have_info) {
    add("Firmware", info.firmware_version);
    add("Serial number", info.serial);
  }

  draw_list("About", rows, -1);
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
