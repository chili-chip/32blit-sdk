#pragma once

#include <string>
#include <vector>

#include "engine/engine.hpp"
#include "graphics/font.hpp"

#include "chilichip.hpp"
#include "layout.hpp"

// The console OS look, shared by the home screen (vgc-os) and the loader's system screens.
// Black background, bone text, one red thing per screen: the selection.
// Colours come from chilichip.hpp, a copy of the design system's tokens (chili-chip/chilichip-brand).
namespace theme {
  constexpr blit::Pen pen(chilichip::Rgb c, int a = 255) { return blit::Pen(c.r, c.g, c.b, a); }

  namespace console = chilichip::console;
  namespace color = chilichip::color;

  const blit::Pen bg = pen(console::bg);
  const blit::Pen surface = pen(console::surface);           // panels only
  const blit::Pen line = pen(console::line);                 // bars, borders
  const blit::Pen text = pen(console::text);
  const blit::Pen muted = pen(console::muted);               // second lines, hints
  const blit::Pen slate = pen(color::slate);                 // rows you can't use now
  const blit::Pen accent = pen(console::accent);             // the selection
  const blit::Pen accent_shade = pen(console::accent_shade); // the strip under red
  const blit::Pen accent_ink = pen(color::chili_ink);        // a bar's track on red
  const blit::Pen on_accent = pen(console::on_accent);       // text on red
  const blit::Pen success = pen(console::success);
  const blit::Pen warning = pen(console::warning);
  const blit::Pen shadow = pen(color::chili_deep);           // hard shadow under panels
  const blit::Pen dim = blit::Pen(0, 0, 0, 158);             // over a screen behind a panel
}

constexpr int status_bar_h = 12; // then a 1 px line
constexpr int row_h = 16;
constexpr int list_top = 16;
constexpr int list_rows = 6;
constexpr int hint_h = 12;       // the last 12 px
constexpr int margin = 4;

// minimal_font at 2x, for the title card of a game without a cover
void draw_text_2x(std::string_view text, blit::Point p);

// fill the screen with the background
void draw_background();

// title on the left in muted text, the battery outline (and a plug on USB) on the right,
// a 1 px line under it
void draw_status_bar(const char *title);

// what A and B do, in the last 12 px. Either can be empty.
void draw_hints(const char *left, const char *right = nullptr);

// text that doesn't fit is cut with "..", then scrolls (see scroll_offset())
void draw_scrolling_text(ScrollingText &state, const std::string &text, const blit::Rect &box);

// a 3 px line: the track in `line`, the value in `text`
void draw_bar(const blit::Rect &r, int value, int max, bool on_accent = false);

struct ListRow {
  ListRow() = default;
  ListRow(std::string label, std::string value = "") : label(std::move(label)), value(std::move(value)) {}

  std::string label;
  std::string value;   // right aligned
  int bar = -1;        // a bar instead of the value when >= 0
  int bar_max = 100;
  bool dim = false;    // can't be used now
};

// one row: 16 px in lists, 14 px in the system menu. The selected row is red with a shade strip,
// a ">" marker and dark text; its label scrolls when it doesn't fit.
void draw_row(const blit::Rect &r, const ListRow &row, bool selected);

// status bar, up to six rows, a scroll bar when there are more, and the hint line
void draw_list(const char *title, const std::vector<ListRow> &rows, int selected, const char *hint_left, const char *hint_right);

// surface fill, 1 px line, hard shadow 4 px down and right, the top right corner cut by 6 px
void draw_panel(const blit::Rect &r);

// a red A button with a shade strip, or an outlined B button. Returns its width.
int draw_button(blit::Point p, char button, const char *label);

// dim the screen and show a panel with a title, a muted detail (three lines at most) and the buttons.
// No B button without no_label.
void draw_dialog(const std::string &title, const std::string &detail, const char *yes_label, const char *no_label);
