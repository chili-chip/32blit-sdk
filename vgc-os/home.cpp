#include <algorithm>
#include <cctype>

#include "32blit.hpp"

#include "app.hpp"
#include "games.hpp"
#include "settings.hpp"
#include "ui.hpp"

using namespace blit;

static Screen current_screen = Screen::home;

// carousel, the last tile is Settings
static int selected = 0;
// the selected cover slides in from slide_from pixels, starting at slide_start
static int slide_from = 0;
static uint32_t slide_start = 0;

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
static std::string dialog_title, dialog_detail;
static const char *dialog_yes_label = "OK";
static const char *dialog_no_label = nullptr; // no B choice without one
static std::function<void()> dialog_on_yes, dialog_on_no;

// titles that scroll when they don't fit
static ScrollingText home_title, info_title;

// with no games the first tile is a "No games yet" page
static bool is_empty_tile(int index) {
  return home_game_count() == 0 && index == 0;
}

static int num_tiles() {
  return std::max(home_game_count(), 1) + 1;
}

static bool is_settings_tile(int index) {
  return index == num_tiles() - 1;
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

void show_confirm(const std::string &title, const std::string &detail, std::function<void()> on_yes, const char *yes_label, const char *no_label, std::function<void()> on_no) {
  dialog_open = true;
  dialog_title = title;
  dialog_detail = detail;
  dialog_yes_label = yes_label;
  dialog_no_label = no_label;
  dialog_on_yes = on_yes;
  dialog_on_no = on_no;
}

void show_message(const std::string &title, const std::string &detail) {
  show_confirm(title, detail, nullptr, "OK", nullptr);
}

void select_game(int game_index) {
  selected = std::max(0, std::min(game_index, num_tiles() - 1));
  slide_from = 0;
}

static void start_game(int index) {
  auto title = get_games()[index].title;
  if(!play_game(index))
    show_message("Couldn't start", title);
}

// home screen

static constexpr int cover_y = status_bar_h + 1;
static constexpr int cover_w = 128, cover_h = 96;

static std::string upper(std::string text) {
  for(auto &c : text)
    c = toupper(c);
  return text;
}

// a game without a cover: its title at 2x on a panel with a cut corner
static void draw_title_card(const Game &game, int x, int y) {
  const int cut = 12;

  screen.pen = theme::surface;
  for(int row = 0; row < cover_h; row++)
    screen.h_span(Point(x, y + row), cover_w - std::max(0, cut - row));

  auto lines = wrap_words(upper(game.title), cover_w - 16, 2);
  if(lines.size() > 3)
    lines.resize(3);

  screen.pen = theme::text;
  int ty = y + 44 - int(lines.size()) * 9;
  for(auto &line : lines) {
    auto fitted = fit_text(line, (cover_w - 12) / 2);
    draw_text_2x(fitted, Point(x + (cover_w - text_width(fitted) * 2) / 2, ty));
    ty += 18;
  }

  // files show which app plays them
  std::string kind = game.is_app_file ? upper(game.author) + " GAME" : "NO COVER";
  kind = fit_text(kind, cover_w - 8);
  screen.pen = theme::muted;
  screen.text(kind, minimal_font, Point(x + (cover_w - text_width(kind)) / 2, y + 82));
}

static void draw_settings_tile(int x, int y) {
  // three sliders
  static const int knobs[] = {36, 12, 26};
  for(int i = 0; i < 3; i++) {
    int ty = y + 31 + i * 16;
    screen.pen = theme::muted;
    screen.rectangle(Rect(x + 36, ty, 56, 2));
    screen.pen = theme::text;
    screen.rectangle(Rect(x + 36 + knobs[i], ty - 4, 6, 10));
  }
}

static void draw_centred(const char *text, int x, int y) {
  screen.text(text, minimal_font, Point(x + (cover_w - text_width(text)) / 2, y));
}

static void draw_empty_tile(int x, int y) {
  // a USB plug
  static const char *const plug[] = {
    "...##......##...",
    "...##......##...",
    "...##......##...",
    "...##......##...",
    "################",
    "################",
    "################",
    "################",
    ".##############.",
    "..############..",
    "...##########...",
    "......####......",
    "......####......",
    "......####......",
  };
  screen.pen = theme::muted;
  for(int row = 0; row < 14; row++)
    for(int col = 0; col < 16; col++)
      if(plug[row][col] == '#')
        screen.pixel(Point(x + 56 + col, y + 14 + row));

  screen.pen = theme::text;
  draw_centred("NO GAMES YET", x, y + 40);
  screen.pen = theme::muted;
  draw_centred("PLUG IN A USB CABLE AND", x, y + 56);
  draw_centred("COPY GAMES TO THE", x, y + 65);
  draw_centred("CHILICHIP DRIVE", x, y + 74);
}

static void draw_tile(int index, int x) {
  const int y = cover_y;

  if(is_settings_tile(index))
    draw_settings_tile(x, y);
  else if(is_empty_tile(index))
    draw_empty_tile(x, y);
  else if(auto cover = get_cover(index)) {
    // 1:1, centred, only scaled down when it's bigger than the space
    Rect src(0, 0, cover->bounds.w, cover->bounds.h);
    if(src.w > cover_w || src.h > cover_h)
      screen.stretch_blit(cover, src, Rect(x, y, cover_w, cover_h));
    else
      screen.blit(cover, src, Point(x + (cover_w - src.w) / 2, y + (cover_h - src.h) / 2));
  } else
    draw_title_card(get_games()[index], x, y);
}

static void render_home() {
  const int w = screen.bounds.w;

  draw_background();

  int offset = slide_offset(slide_from, now() - slide_start);
  for(int d : {-1, 1, 0}) {
    int index = selected + d;
    if(index < 0 || index >= num_tiles() || (d != 0 && offset == 0))
      continue;
    draw_tile(index, (w - cover_w) / 2 + d * cover_w + offset);
  }

  // covers slide under the bars
  screen.pen = theme::bg;
  screen.rectangle(Rect(0, 0, w, cover_y));
  screen.rectangle(Rect(0, cover_y + cover_h, w, screen.bounds.h - cover_y - cover_h));

  draw_status_bar("CHILICHIP");

  // where you are: a bar and a count
  const int bar_y = cover_y + cover_h;
  auto pos = position_bar(selected, num_tiles(), w);
  screen.pen = theme::line;
  screen.rectangle(Rect(0, bar_y, w, 2));
  screen.pen = theme::accent;
  screen.rectangle(Rect(pos.pos, bar_y, pos.size, 2));

  std::string count = std::to_string(selected + 1) + "/" + std::to_string(num_tiles());
  int count_w = text_width(count);
  screen.pen = theme::muted;
  screen.text(count, minimal_font, Point(w - margin - count_w, bar_y + 3));

  std::string title = is_settings_tile(selected) ? "Settings" : is_empty_tile(selected) ? "Welcome" : get_games()[selected].title;
  screen.pen = theme::text;
  draw_scrolling_text(home_title, title, Rect(margin, bar_y + 3, w - margin * 3 - count_w, 9));

  if(!is_settings_tile(selected) && !is_empty_tile(selected)) {
    screen.pen = theme::muted;
    screen.text(fit_text(get_games()[selected].author, 80), minimal_font, Point(margin, bar_y + 11));
    screen.text("Y INFO", minimal_font, Point(w - margin - text_width("Y INFO"), bar_y + 11));
  }
}

static void move_to(int index) {
  if(index < 0 || index >= num_tiles() || index == selected)
    return;

  // the new cover comes in from the side it's on
  int from = (index > selected ? cover_w : -cover_w) + slide_offset(slide_from, now() - slide_start);
  slide_from = std::max(-cover_w, std::min(from, cover_w));
  slide_start = now();
  selected = index;
}

static void update_home(uint32_t pressed) {
  if(pressed & Button::DPAD_LEFT)
    move_to(selected - 1);

  if(pressed & Button::DPAD_RIGHT)
    move_to(selected + 1);

  trim_covers(selected, 2);

  if(pressed & Button::A) {
    if(is_settings_tile(selected))
      go_to(Screen::settings);
    else if(!is_empty_tile(selected))
      start_game(selected);
  }

  if(pressed & Button::Y && !is_settings_tile(selected) && !is_empty_tile(selected))
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
      show_confirm("Delete game?", game.title + " will be removed.", [] {
        auto title = get_games()[info_game].title;
        bool had_saves = has_save_data(title);

        delete_game(info_game);
        after_delete();

        // B keeps them, like backing out of any other question
        if(had_saves) {
          show_confirm("Delete save data too?", "Progress in " + title + " will be lost.", [title] {
            if(!delete_save_data(title))
              show_message("Couldn't delete", "Some of the save data is still there.");
          }, "Delete", "Keep");
        }
      }, "Delete", "Keep");
    } else if(info_action == DeleteSave) {
      if(!has_save_data(info_game))
        show_message("No save data", "This game has not saved anything.");
      else {
        show_confirm("Delete save data?", "Progress in " + game.title + " will be lost.", [] {
          if(delete_save_data(info_game))
            show_message("Save data deleted", "");
          else
            show_message("Couldn't delete", "Some of the save data is still there.");
        }, "Delete", "Keep");
      }
    }
  }

  if(pressed & Button::B)
    go_to(info_back);
}

static void render_info() {
  const int w = screen.bounds.w;
  const int text_w = w - margin * 2;
  auto &game = get_games()[info_game];

  draw_background();
  draw_status_bar("GAME INFO");

  screen.pen = theme::text;
  draw_scrolling_text(info_title, game.title, Rect(margin, 16, text_w, 9));

  std::string details = game.author;
  if(!game.version.empty())
    details += details.empty() ? game.version : "  " + game.version;
  screen.pen = theme::muted;
  screen.text(fit_text(details, text_w), minimal_font, Point(margin, 25));

  screen.pen = theme::line;
  screen.h_span(Point(margin, 35), text_w);

  // three lines of description, cut when there is more
  auto lines = wrap_words(game.description, text_w);
  screen.pen = theme::muted;
  for(int i = 0; i < 3 && i < int(lines.size()); i++) {
    auto line = lines[i];
    if(i == 2 && lines.size() > 3)
      line = fit_text(line + " ...", text_width(line));
    screen.text(line, minimal_font, Point(margin, 38 + i * 9));
  }

  ListRow rows[InfoActionCount] = {{"Play"}, {"Delete game", format_size(game.size)}, {"Delete save data"}};
  rows[DeleteSave].dim = !has_save_data(info_game);

  for(int i = 0; i < InfoActionCount; i++)
    draw_row(Rect(0, 66 + i * row_h, w, row_h), rows[i], i == info_action);

  draw_hints("A SELECT", "B BACK");
}

// 32blit

void init() {
  set_screen_mode(ScreenMode::hires);
  load_games();
  selected = 0;
}

void update(uint32_t time) {
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
    draw_dialog(dialog_title, dialog_detail, dialog_yes_label, dialog_no_label);
}
