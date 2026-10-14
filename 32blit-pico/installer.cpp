#include <cstring>
#include <string>
#include <vector>

#include "pico/time.h"

#include "blit_launch.hpp"
#include "config.h"
#include "display.hpp"
#include "file.hpp"
#include "installer.hpp"
#include "system_menu.hpp"
#include "usb.hpp"

#include "engine/api_private.hpp"
#include "engine/engine.hpp"

#include "theme.hpp"

#ifdef BUILD_LOADER

using namespace blit;

#ifndef CONSOLE_NAME
#define CONSOLE_NAME USB_PRODUCT_STR
#endif

extern int (*do_tick)(uint32_t time);
void disable_user_code();

enum class ItemState {
  Waiting,
  Installing,
  Done
};

struct InstallItem {
  std::string filename;
  ItemState state = ItemState::Waiting;
  InstallResult result = InstallResult::Failed;
  uint32_t progress = 0, total = 0;
};

static bool scan_pending = true; // check once at boot
static bool active = false;

static std::vector<InstallItem> items;
static int current_item = 0;
static bool finished = false, had_errors = false;
static uint32_t finish_time = 0;
static bool a_pressed = false;

static uint32_t now_ms() {
  return to_ms_since_boot(get_absolute_time());
}

static bool is_blit_file(const std::string &name) {
  if(name.size() < 6 || name[0] == '.') // hidden, includes macOS "._" files
    return false;

  auto ext = name.substr(name.size() - 5);
  for(auto &c : ext)
    c = tolower(c);

  return ext == ".blit";
}

static const char *result_text(InstallResult res) {
  switch(res) {
    case InstallResult::Installed:        return "Installed";
    case InstallResult::AlreadyInstalled: return "Already installed";
    case InstallResult::NotAGame:         return "Not a " CONSOLE_NAME " game";
    case InstallResult::NeedsUpdate:      return "Needs a system update";
    case InstallResult::Damaged:          return "File damaged";
    case InstallResult::NoSpace:          return "Not enough space";
    case InstallResult::Failed:           break;
  }
  return "Install failed";
}

static bool is_error(InstallResult res) {
  return res != InstallResult::Installed && res != InstallResult::AlreadyInstalled;
}

static void installer_render(uint32_t time) {
  const int w = screen.bounds.w, h = screen.bounds.h;
  const int footer_h = 13, row_h = 20;

  draw_background();
  draw_status_bar(finished ? "Install complete" : "Installing games");

  // keep the current game in view
  int top = status_bar_h + 3;
  int visible = (h - top - footer_h) / row_h;
  int first = 0;
  if(current_item >= visible)
    first = current_item - visible + 1;

  for(int i = first; i < int(items.size()) && i < first + visible; i++) {
    auto &item = items[i];
    int y = top + (i - first) * row_h;

    // name without the extension
    auto name = item.filename.substr(0, item.filename.size() - 5);
    screen.pen = theme::text;
    screen.text(fit_text(name, w - 8), minimal_font, Rect(4, y, w - 8, 9), true, TextAlign::top_left);

    Rect status(4, y + 10, w - 8, 7);

    if(item.state == ItemState::Waiting) {
      screen.pen = theme::faint_text;
      screen.text("Waiting", minimal_font, status, true, TextAlign::top_left);
    } else if(item.state == ItemState::Installing) {
      draw_bar(Rect(status.x, status.y + 1, status.w, 5), item.progress, item.total);
    } else {
      if(is_error(item.result))
        screen.pen = theme::hot;
      else if(item.result == InstallResult::Installed)
        screen.pen = theme::good;
      else
        screen.pen = theme::dim_text;

      screen.text(result_text(item.result), minimal_font, status, true, TextAlign::top_left);
    }
  }

  // footer
  if(!finished) {
    char buf[32];
    snprintf(buf, sizeof(buf), "%i of %i", current_item + 1, int(items.size()));
    screen.pen = theme::dim_text;
    screen.text(buf, minimal_font, Rect(0, h - footer_h, w, footer_h), true, TextAlign::center_center);
  } else if(had_errors)
    draw_hints({{'A', "Continue"}}, h - 11);
}

// draw a frame now, used while the main loop is blocked by an install
static void refresh() {
  blit::render = installer_render; // starting a flash write resets it
  update_display(now_ms());
  usb_service();
}

static void present() {
  // update_display only draws every 20ms without vsync, keep trying until it does
  auto start = now_ms();
  while(now_ms() - start < 25)
    refresh();
}

static int installer_tick(uint32_t time) {
  auto &buttons = api_data.buttons;
  if(buttons.pressed & Button::A)
    a_pressed = true;

  bool done = had_errors ? (a_pressed && (buttons.released & Button::A)) : time - finish_time >= 1500;

  if(done) {
    items.clear();
    // back to the home screen, which re-scans the drive and installed games
    disable_user_code();
    quit_to_launcher();
  }

  return 10;
}

static void run_install(std::vector<InstallItem> &&new_items) {
  items = std::move(new_items);
  current_item = 0;
  finished = had_errors = false;
  a_pressed = false;
  active = true;

  close_open_files();
  launch_pre_init();
  disable_user_code();

  // nothing has been launched, borrow game RAM for a framebuffer
  if(!screen_fb) {
    extern char __StackTop;
    set_framebuffer((uint8_t *)&__StackTop, DISPLAY_WIDTH * DISPLAY_HEIGHT * 2, Size(DISPLAY_WIDTH, DISPLAY_HEIGHT));
  }
  ::set_screen_mode(ScreenMode::hires);

  present();

  bool removed = false;

  for(current_item = 0; current_item < int(items.size()); current_item++) {
    auto &item = items[current_item];
    item.state = ItemState::Installing;
    present();

    item.result = install_blit_file(item.filename.c_str(), [&item](uint32_t done, uint32_t total) {
      item.progress = done;
      item.total = total;
      refresh();
    });

    item.state = ItemState::Done;

    if(is_error(item.result))
      had_errors = true;
    else {
      // free the space on the drive, and the macOS metadata file next to it
      ::remove_file(item.filename);
      auto apple_double = "._" + item.filename;
      if(::file_exists(apple_double))
        ::remove_file(apple_double);
      removed = true;
    }

    present();
  }

  current_item = int(items.size()) - 1;
  finished = true;
  finish_time = now_ms();
  active = false;

  if(removed)
    usb_storage_changed();

  present();

  // wait on the result screen until it times out or is dismissed
  blit::render = installer_render;
  do_tick = installer_tick;
}

void installer_request_scan() {
  scan_pending = true;
}

void update_installer() {
  // still installing, or showing the results
  if(!scan_pending || active || do_tick == installer_tick)
    return;

  // only touch the drive's files when the computer isn't using it
  if(usb_storage_in_use())
    return;

  // wait for the remount after the computer's writes
  if(!is_fs_available())
    return;

  // install on the home screen, never under a running game
  if(!is_launcher_running())
    return;

  scan_pending = false;

  std::vector<InstallItem> new_items;
  list_files("/", [&new_items](FileInfo &info) {
    if(!(info.flags & FileFlags::directory) && is_blit_file(info.name)) {
      InstallItem item;
      item.filename = info.name;
      new_items.push_back(item);
    }
  });

  if(!new_items.empty())
    run_install(std::move(new_items));
}

bool installer_active() {
  return active;
}

#endif
