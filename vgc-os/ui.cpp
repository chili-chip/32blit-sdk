#include <cstdio>

#include "ui.hpp"

using namespace blit;

static void draw_battery(Point p, int level) {
  // 12x7 body with a nub on the right
  screen.pen = theme::dim_text;
  screen.rectangle(Rect(p.x, p.y, 12, 7));
  screen.rectangle(Rect(p.x + 12, p.y + 2, 1, 3));
  screen.pen = theme::background;
  screen.rectangle(Rect(p.x + 1, p.y + 1, 10, 5));

  if(level < 0)
    return;

  screen.pen = level <= 15 ? theme::bad : theme::good;
  screen.rectangle(Rect(p.x + 1, p.y + 1, (level * 10 + 50) / 100, 5));
}

static void draw_plug(Point p) {
  // two prongs and a body
  screen.pen = theme::text;
  screen.rectangle(Rect(p.x + 1, p.y, 1, 2));
  screen.rectangle(Rect(p.x + 4, p.y, 1, 2));
  screen.rectangle(Rect(p.x, p.y + 2, 6, 3));
  screen.rectangle(Rect(p.x + 2, p.y + 5, 2, 2));
}

void draw_status_bar(const char *title) {
  const int w = screen.bounds.w;

  screen.pen = theme::panel;
  screen.rectangle(Rect(0, 0, w, status_bar_h));

  if(title) {
    screen.pen = theme::text;
    screen.text(title, minimal_font, Rect(3, 0, w - 50, status_bar_h), true, TextAlign::center_left);
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
    screen.text(buf, minimal_font, Rect(x, 0, text_w, status_bar_h), true, TextAlign::center_left);
  }

  if(is_usb_connected())
    draw_plug(Point(x - 9, 2));
}

std::string fit_text(const std::string &text, int max_w) {
  if(screen.measure_text(text, minimal_font).w <= max_w)
    return text;

  std::string out = text;
  while(!out.empty() && screen.measure_text(out + "..", minimal_font).w > max_w)
    out.pop_back();

  return out + "..";
}

void draw_bar(const Rect &r, int value, int max) {
  screen.pen = theme::bar_back;
  screen.rectangle(r);
  screen.pen = theme::text;
  screen.rectangle(Rect(r.x, r.y, max ? r.w * value / max : 0, r.h));
}

uint32_t get_pressed(uint32_t time) {
  static uint32_t held_since = 0, last_repeat = 0;
  const uint32_t dpad = Button::DPAD_UP | Button::DPAD_DOWN | Button::DPAD_LEFT | Button::DPAD_RIGHT;

  uint32_t pressed = buttons.pressed;

  if(pressed & dpad)
    held_since = last_repeat = time;
  else if(buttons.state & dpad) {
    if(time - held_since > 400 && time - last_repeat > 110) {
      pressed |= buttons.state & dpad;
      last_repeat = time;
    }
  }

  return pressed;
}

void draw_list(const char *title, const std::vector<ListRow> &rows, int selected) {
  const int w = screen.bounds.w, h = screen.bounds.h;
  const int row_h = 13;
  const int top = status_bar_h + 2;
  const int visible = (h - top) / row_h;

  screen.pen = theme::background;
  screen.clear();
  draw_status_bar(title);

  int first = 0;
  if(selected >= visible)
    first = selected - visible + 1;

  for(int i = first; i < int(rows.size()) && i < first + visible; i++) {
    auto &row = rows[i];
    Rect r(0, top + (i - first) * row_h, w, row_h);

    if(i == selected) {
      screen.pen = theme::highlight;
      screen.rectangle(r);
    }

    Rect inner(r.x + 4, r.y, r.w - 8, r.h);
    const int value_w = row.bar >= 0 ? 40 : screen.measure_text(row.value, minimal_font).w;

    screen.pen = row.dim ? theme::dim_text : theme::text;
    screen.text(fit_text(row.label, inner.w - value_w - 4), minimal_font, inner, true, TextAlign::center_left);

    if(row.bar >= 0)
      draw_bar(Rect(inner.x + inner.w - 40, inner.y + row_h / 2 - 2, 40, 4), row.bar, row.bar_max);
    else if(!row.value.empty()) {
      screen.pen = theme::dim_text;
      screen.text(row.value, minimal_font, inner, true, TextAlign::center_right);
    }
  }
}

void draw_dialog(const std::string &message, const char *hint) {
  const int w = screen.bounds.w, h = screen.bounds.h;
  const int margin = 8;
  const int text_w = w - margin * 2 - 8;

  auto wrapped = screen.wrap_text(message, text_w, minimal_font);
  int lines = 1;
  for(auto c : wrapped)
    if(c == '\n')
      lines++;

  Rect box(margin, 0, w - margin * 2, lines * 9 + 26);
  box.y = (h - box.h) / 2;

  screen.pen = Pen(0, 0, 0, 160);
  screen.rectangle(Rect(0, 0, w, h));
  screen.pen = theme::dim_text;
  screen.rectangle(box);
  screen.pen = theme::panel;
  screen.rectangle(Rect(box.x + 1, box.y + 1, box.w - 2, box.h - 2));

  screen.pen = theme::text;
  screen.text(wrapped, minimal_font, Rect(box.x + 4, box.y + 5, text_w, lines * 9), true, TextAlign::top_center);

  screen.pen = theme::dim_text;
  screen.text(hint, minimal_font, Rect(box.x, box.y + box.h - 14, box.w, 10), true, TextAlign::center_center);
}

std::string format_size(uint32_t bytes) {
  char buf[16];
  if(bytes >= 1024 * 1024)
    snprintf(buf, sizeof(buf), "%u.%u MB", unsigned(bytes / (1024 * 1024)), unsigned(bytes % (1024 * 1024) * 10 / (1024 * 1024)));
  else
    snprintf(buf, sizeof(buf), "%u KB", unsigned((bytes + 1023) / 1024));
  return buf;
}
