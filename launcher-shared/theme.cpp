#include <algorithm>
#include <cstdio>

#include "theme.hpp"

#include "engine/system.hpp"

using namespace blit;

Pen mix(const Pen &a, const Pen &b, int t, int max) {
  if(max <= 0)
    return a;
  auto lerp = [t, max](int x, int y) { return uint8_t(x + (y - x) * t / max); };
  return Pen(lerp(a.r, b.r), lerp(a.g, b.g), lerp(a.b, b.b), lerp(a.a, b.a));
}

void draw_background() {
  const int w = screen.bounds.w, h = screen.bounds.h;
  for(int y = 0; y < h; y++) {
    screen.pen = mix(theme::background_top, theme::background_bottom, y, h - 1);
    screen.h_span(Point(0, y), w);
  }
}

void draw_round_rect(const Rect &r, const Pen &left, const Pen &right) {
  if(r.w <= 0 || r.h <= 0)
    return;

  for(int x = 0; x < r.w; x++) {
    // one pixel cut from each corner
    int inset = (x == 0 || x == r.w - 1) && r.h > 2 ? 1 : 0;
    screen.pen = mix(left, right, x, r.w - 1);
    screen.v_span(Point(r.x + x, r.y + inset), r.h - inset * 2);
  }
}

void draw_round_rect(const Rect &r, const Pen &colour) {
  draw_round_rect(r, colour, colour);
}

static void draw_battery(Point p, int level) {
  // 12x7 body with a nub on the right
  screen.pen = theme::text;
  screen.rectangle(Rect(p.x, p.y, 12, 7));
  screen.rectangle(Rect(p.x + 12, p.y + 2, 1, 3));
  screen.pen = theme::chili;
  screen.rectangle(Rect(p.x + 1, p.y + 1, 10, 5));

  if(level < 0)
    return;

  screen.pen = level <= 15 ? theme::pepper : theme::text;
  screen.rectangle(Rect(p.x + 2, p.y + 2, (level * 8 + 50) / 100, 3));
}

static void draw_plug(Point p) {
  // two prongs and a body
  screen.pen = theme::text;
  screen.rectangle(Rect(p.x + 1, p.y, 1, 2));
  screen.rectangle(Rect(p.x + 4, p.y, 1, 2));
  screen.rectangle(Rect(p.x, p.y + 2, 6, 3));
  screen.rectangle(Rect(p.x + 2, p.y + 5, 2, 2));
}

static void draw_chili_icon(Point p) {
  static const char *const rows[] = {
    "....gg",
    "...gg.",
    "..oooo",
    ".yooo.",
    "yooo..",
    "ooo...",
    "oo....",
    "o.....",
  };

  for(int y = 0; y < 8; y++) {
    for(int x = 0; rows[y][x]; x++) {
      switch(rows[y][x]) {
        case 'g': screen.pen = theme::stem; break;
        case 'o': screen.pen = theme::pepper; break;
        case 'y': screen.pen = theme::text; break;
        default: continue;
      }
      screen.pixel(Point(p.x + x, p.y + y));
    }
  }
}

void draw_status_bar(const char *title) {
  const int w = screen.bounds.w;

  draw_round_rect(Rect(0, 0, w, status_bar_h), theme::chili, Pen(180, 16, 40));
  screen.pen = theme::chili;
  screen.rectangle(Rect(0, 0, w, 2));
  screen.pen = theme::hot;
  screen.h_span(Point(0, status_bar_h - 1), w);

  screen.pen = theme::text;
  if(title)
    screen.text(title, fat_font, Rect(4, 0, w - 50, status_bar_h - 1), true, TextAlign::center_left);
  else {
    draw_chili_icon(Point(3, 1));
    screen.text("ChiliChip", fat_font, Rect(12, 0, w - 50, status_bar_h - 1), true, TextAlign::center_left);
  }

  int level = get_battery_level();
  int x = w - 15;
  draw_battery(Point(x, 2), level);

  if(level >= 0) {
    char buf[16];
    snprintf(buf, sizeof(buf), "%i%%", level);
    auto text_w = screen.measure_text(buf, minimal_font).w;
    x -= text_w + 2;
    screen.pen = theme::text;
    screen.text(buf, minimal_font, Rect(x, 0, text_w, status_bar_h - 1), true, TextAlign::center_left);
  }

  if(is_usb_connected())
    draw_plug(Point(x - 9, 2));
}

static Pen button_colour(char button) {
  switch(button) {
    case 'A': return theme::chili;
    case 'B': return theme::hot;
    default: return theme::pepper;
  }
}

void draw_hints(std::initializer_list<ButtonHint> hints, int y) {
  const int gap = 8, button_w = 9, label_gap = 3;

  int total = 0;
  for(auto &hint : hints)
    total += button_w + label_gap + screen.measure_text(hint.label, minimal_font).w + gap;
  total -= gap;

  int x = (screen.bounds.w - total) / 2;
  for(auto &hint : hints) {
    Rect b(x, y, button_w, 9);
    draw_round_rect(b, button_colour(hint.button));

    char letter[2] = {hint.button, 0};
    screen.pen = hint.button == 'Y' ? theme::background_bottom : theme::text;
    screen.text(letter, minimal_font, Rect(b.x + 1, b.y, b.w, b.h), true, TextAlign::center_center);

    x += button_w + label_gap;
    auto label_w = screen.measure_text(hint.label, minimal_font).w;
    screen.pen = theme::text;
    screen.text(hint.label, minimal_font, Rect(x, y, label_w, 9), true, TextAlign::center_left);
    x += label_w + gap;
  }
}

void draw_panel(const Rect &r) {
  // hot outline, dark body, chili stripe on top
  draw_round_rect(r, theme::chili, theme::hot);
  draw_round_rect(Rect(r.x + 1, r.y + 3, r.w - 2, r.h - 4), theme::panel);
}

void draw_selection(const Rect &r) {
  draw_round_rect(r, theme::chili, theme::hot);
  screen.pen = theme::pepper;
  screen.rectangle(Rect(r.x, r.y + 2, 2, std::max<int32_t>(1, r.h - 4)));
}

std::string fit_text(const std::string &text, int max_w, const Font &font) {
  if(screen.measure_text(text, font).w <= max_w)
    return text;

  std::string out = text;
  while(!out.empty() && screen.measure_text(out + "..", font).w > max_w)
    out.pop_back();

  return out + "..";
}

void draw_bar(const Rect &r, int value, int max, bool on_highlight) {
  draw_round_rect(r, on_highlight ? theme::background_bottom : theme::bar_back);

  int fill = max ? r.w * std::min(value, max) / max : 0;
  if(fill <= 0)
    return;

  // the colour runs from chili to pepper across the whole bar
  Rect f(r.x, r.y, std::max(fill, 2), r.h);
  if(on_highlight)
    draw_round_rect(f, theme::text);
  else
    draw_round_rect(f, theme::chili, mix(theme::chili, theme::pepper, f.w, r.w));
}
