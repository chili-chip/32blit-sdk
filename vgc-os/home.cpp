#include <cmath>

#include "32blit.hpp"

#include "app.hpp"
#include "games.hpp"
#include "settings.hpp"
#include "ui.hpp"

using namespace blit;

static Screen current_screen = Screen::home;

// carousel, the last tile is Settings
static int selected = 0;
static float slide = 0.0f; // slots the carousel still has to move, animates to 0

// info screen
enum InfoAction {
  Play,
  DeleteGame,
  DeleteSave,
  InfoActionCount
};
static int info_game = 0;
static int info_action = 0;
static Screen info_back = Screen::home;

// dialog over the current screen
static bool dialog_open = false;
static std::string dialog_message;
static const char *dialog_yes_label = "OK";
static const char *dialog_no_label = nullptr; // no B choice without one
static std::function<void()> dialog_on_yes, dialog_on_no;

static int num_tiles() {
  return home_game_count() + 1;
}

static bool is_settings_tile(int index) {
  return index == home_game_count();
}

void go_to(Screen screen) {
  current_screen = screen;
  settings_enter(screen);
}

void open_info(int game_index, Screen back) {
  info_game = game_index;
  info_action = Play;
  info_back = back;
  current_screen = Screen::info;
}

void show_confirm(const std::string &message, std::function<void()> on_yes, const char *yes_label, const char *no_label, std::function<void()> on_no) {
  dialog_open = true;
  dialog_message = message;
  dialog_yes_label = yes_label;
  dialog_no_label = no_label;
  dialog_on_yes = on_yes;
  dialog_on_no = on_no;
}

void show_message(const std::string &message) {
  show_confirm(message, nullptr, "OK", nullptr);
}

void select_game(int game_index) {
  selected = std::max(0, std::min(game_index, num_tiles() - 1));
  slide = 0.0f;
}

static void start_game(int index) {
  auto title = get_games()[index].title;
  if(!play_game(index))
    show_message("Couldn't start " + title);
}

// carousel

static Rect tile_rect(float r) {
  const int w = screen.bounds.w;
  const int big_w = w * 3 / 4, big_h = big_w * 3 / 4;
  const int small_w = big_w / 2, small_h = big_h / 2;
  const int centre_y = status_bar_h + 6 + big_h / 2;
  const float side_dx = w / 2 + 4;

  float a = std::min(std::abs(r), 1.0f);
  float tw = big_w + (small_w - big_w) * a;
  float th = big_h + (small_h - big_h) * a;

  float dx = r * side_dx;
  if(std::abs(r) > 1.0f)
    dx = (r > 0 ? 1 : -1) * (side_dx + (std::abs(r) - 1.0f) * (small_w + 8));

  int cx = w / 2 + int(std::round(dx));
  return Rect(cx - int(tw) / 2, centre_y - int(th) / 2, int(tw), int(th));
}

static Pen title_colour(const std::string &title) {
  uint32_t hash = 2166136261u;
  for(auto c : title)
    hash = (hash ^ uint8_t(c)) * 16777619u;

  static const Pen colours[] = {
    theme::chili, theme::hot, Pen(200, 20, 90), Pen(150, 30, 160), Pen(60, 150, 50), Pen(230, 150, 0)
  };
  return colours[hash % (sizeof(colours) / sizeof(colours[0]))];
}

static void draw_card(const Rect &r, Pen colour, const std::string &label) {
  draw_round_rect(r, colour, mix(colour, theme::background_bottom, 1, 2));

  if(r.w < 40)
    return;

  screen.pen = theme::text;
  auto wrapped = screen.wrap_text(label, r.w - 8, minimal_font);
  screen.text(wrapped, minimal_font, Rect(r.x + 4, r.y, r.w - 8, r.h), true, TextAlign::center_center);
}

static void draw_gear(const Rect &r) {
  draw_round_rect(r, theme::chili, theme::hot);

  Point c(r.x + r.w / 2, r.y + r.h / 2);
  int radius = std::min(r.w, r.h) / 4;

  screen.pen = theme::text;
  for(int i = 0; i < 8; i++) {
    float a = i * pi / 4;
    Point p(c.x + int(std::cos(a) * radius * 1.3f), c.y + int(std::sin(a) * radius * 1.3f));
    screen.circle(p, std::max(1, radius / 3));
  }
  screen.circle(c, radius);
  screen.pen = mix(theme::chili, theme::hot, 1, 2);
  screen.circle(c, std::max(1, radius / 2));
}

static void draw_tile(int index, const Rect &r, bool focused) {
  if(focused) {
    // a hot glow around the selected tile
    draw_round_rect(Rect(r.x - 3, r.y - 3, r.w + 6, r.h + 6), mix(theme::chili, theme::background_top, 1, 2), mix(theme::hot, theme::background_top, 1, 2));
    draw_round_rect(Rect(r.x - 2, r.y - 2, r.w + 4, r.h + 4), theme::chili, theme::pepper);
  }

  if(is_settings_tile(index))
    draw_gear(r);
  else if(auto cover = get_cover(index))
    screen.stretch_blit(cover, Rect(0, 0, cover->bounds.w, cover->bounds.h), r);
  else {
    auto &title = get_games()[index].title;
    draw_card(r, title_colour(title), title);
  }

  if(!focused) {
    screen.pen = theme::shade;
    screen.rectangle(r);
  }
}

static void render_home() {
  const int w = screen.bounds.w, h = screen.bounds.h;

  draw_background();

  // furthest first so the selected tile ends up on top
  for(int dist = 2; dist >= 0; dist--) {
    for(int side : {-1, 1}) {
      if(dist == 0 && side == 1)
        continue;

      int index = selected + dist * side;
      if(index < 0 || index >= num_tiles())
        continue;

      float r = index - selected + slide;
      draw_tile(index, tile_rect(r), dist == 0 && std::abs(slide) < 0.5f);
    }
  }

  // position dots under the tile
  auto big = tile_rect(0);
  const int count = num_tiles();
  int dots_y = big.y + big.h + 4;
  if(count <= 16) {
    const int spacing = 5;
    int x = w / 2 - ((count - 1) * spacing + 4) / 2;
    for(int i = 0; i < count; i++) {
      if(i == selected) {
        draw_round_rect(Rect(x, dots_y, 6, 2), theme::hot, theme::pepper);
        x += 7;
      } else {
        screen.pen = theme::faint_text;
        screen.rectangle(Rect(x, dots_y, 2, 2));
        x += spacing;
      }
    }
  }

  // title of the selected tile
  Rect title_r(4, dots_y + 4, w - 8, 9);
  Rect sub_r(4, title_r.y + 10, w - 8, 9);

  if(is_settings_tile(selected)) {
    screen.pen = theme::text;
    screen.text("Settings", fat_font, title_r, true, TextAlign::top_center);

    if(home_game_count() == 0) {
      screen.pen = theme::hot;
      screen.text("Copy games to the USB drive", minimal_font, sub_r, true, TextAlign::top_center);
    }
  } else {
    auto &game = get_games()[selected];
    screen.pen = theme::text;
    screen.text(fit_text(game.title, title_r.w, fat_font), fat_font, title_r, true, TextAlign::top_center);
    screen.pen = theme::hot;
    screen.text(fit_text(game.author, sub_r.w), minimal_font, sub_r, true, TextAlign::top_center);
  }

  if(is_settings_tile(selected))
    draw_hints({{'A', "Open"}}, h - 10);
  else
    draw_hints({{'A', "Play"}, {'Y', "Info"}}, h - 10);

  draw_status_bar();
}

static void update_home(uint32_t pressed) {
  if(pressed & Button::DPAD_LEFT && selected > 0) {
    selected--;
    slide -= 1.0f;
  }

  if(pressed & Button::DPAD_RIGHT && selected < num_tiles() - 1) {
    selected++;
    slide += 1.0f;
  }

  trim_covers(selected, 2);

  if(pressed & Button::A) {
    if(is_settings_tile(selected))
      go_to(Screen::settings);
    else
      start_game(selected);
  }

  if(pressed & Button::Y && !is_settings_tile(selected))
    open_info(selected, Screen::home);

  if(pressed & Button::MENU)
    go_to(Screen::settings);
}

// game info

static void after_delete() {
  // stay on the same position, or the last game
  int count = home_game_count();
  if(info_back == Screen::home)
    select_game(std::min(info_game, count));
  else
    select_game(selected); // keep the carousel in range, deleting an app also removes its files' tiles

  go_to(info_back);
}

static void update_info(uint32_t pressed) {
  auto &game = get_games()[info_game];

  if(pressed & Button::DPAD_UP)
    info_action = (info_action + InfoActionCount - 1) % InfoActionCount;
  if(pressed & Button::DPAD_DOWN)
    info_action = (info_action + 1) % InfoActionCount;

  if(pressed & Button::A) {
    if(info_action == Play)
      start_game(info_game);
    else if(info_action == DeleteGame) {
      show_confirm("Delete " + game.title + "?", [] {
        auto title = get_games()[info_game].title;
        bool had_saves = has_save_data(title);

        delete_game(info_game);
        after_delete();

        // B keeps them, like backing out of any other question
        if(had_saves) {
          show_confirm("Also delete the save data for " + title + "?", [title] {
            if(!delete_save_data(title))
              show_message("Couldn't delete all of the save data");
          }, "Delete", "Keep");
        }
      }, "Delete", "Cancel");
    } else if(info_action == DeleteSave) {
      if(!has_save_data(info_game))
        show_message("No save data");
      else {
        show_confirm("Delete the save data for " + game.title + "?", [] {
          if(delete_save_data(info_game))
            show_message("Save data deleted");
          else
            show_message("Couldn't delete all of the save data");
        }, "Delete", "Cancel");
      }
    }
  }

  if(pressed & Button::B)
    go_to(info_back);
}

static void render_info() {
  const int w = screen.bounds.w, h = screen.bounds.h;
  auto &game = get_games()[info_game];

  draw_background();
  draw_status_bar("Game info");

  int y = status_bar_h + 4;
  screen.pen = theme::text;
  screen.text(fit_text(game.title, w - 8, fat_font), fat_font, Point(4, y));
  y += 10;

  screen.pen = theme::hot;
  std::string details = game.author;
  if(!game.version.empty())
    details += details.empty() ? game.version : "  " + game.version;
  screen.text(fit_text(details, w - 8), minimal_font, Point(4, y));
  y += 9;
  screen.pen = theme::dim_text;
  screen.text(format_size(game.size), minimal_font, Point(4, y));
  y += 10;

  screen.pen = theme::chili;
  screen.h_span(Point(4, y), w - 8);
  y += 3;

  // as much of the description as fits above the actions
  const int row_h = 13;
  const int actions_y = h - InfoActionCount * row_h - 1;
  auto wrapped = screen.wrap_text(game.description, w - 8, minimal_font);
  int max_lines = std::max(0, (actions_y - 2 - y) / 9);
  size_t end = 0;
  for(int line = 0; line < max_lines && end != std::string::npos; line++)
    end = wrapped.find('\n', line ? end + 1 : 0);
  wrapped = max_lines ? wrapped.substr(0, end) : "";

  screen.pen = theme::text;
  screen.text(wrapped, minimal_font, Rect(4, y, w - 8, actions_y - y - 2), true, TextAlign::top_left);

  static const char *const labels[InfoActionCount] = {"Play", "Delete game", "Delete save data"};

  for(int i = 0; i < InfoActionCount; i++) {
    Rect r(0, actions_y + i * row_h, w, row_h);
    bool is_selected = i == info_action;
    if(is_selected) {
      draw_selection(Rect(r.x + 2, r.y, r.w - 4, r.h - 1));
    }

    screen.pen = i == DeleteSave && !has_save_data(info_game) && !is_selected ? theme::faint_text : theme::text;
    screen.text(labels[i], minimal_font, Rect(r.x + 7, r.y, r.w - 12, r.h - 1), true, TextAlign::center_left);
  }
}

// 32blit

void init() {
  set_screen_mode(ScreenMode::hires);
  load_games();
  selected = 0;
}

void update(uint32_t time) {
  // slide towards the selected tile
  slide *= 0.7f;
  if(std::abs(slide) < 0.01f)
    slide = 0.0f;

  uint32_t pressed = get_pressed(time);

  if(dialog_open) {
    // the answer may open another dialog
    std::function<void()> answer;
    if(pressed & Button::A) {
      dialog_open = false;
      answer = dialog_on_yes;
    } else if(pressed & Button::B) {
      dialog_open = false;
      answer = dialog_on_no;
    }

    if(answer)
      answer();
    return;
  }

  switch(current_screen) {
    case Screen::home:
      update_home(pressed);
      break;
    case Screen::info:
      update_info(pressed);
      break;
    default:
      settings_update(current_screen, pressed);
      break;
  }
}

void render(uint32_t time) {
  switch(current_screen) {
    case Screen::home:
      render_home();
      break;
    case Screen::info:
      render_info();
      break;
    default:
      settings_render(current_screen);
      break;
  }

  if(dialog_open)
    draw_dialog(dialog_message, dialog_yes_label, dialog_no_label);
}
