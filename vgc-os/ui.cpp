#include <algorithm>
#include <cstdio>

#include "ui.hpp"

using namespace blit;

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
  const int top = status_bar_h + 3;
  const int visible = (h - top) / row_h;

  draw_background();
  draw_status_bar(title);

  int first = 0;
  if(selected >= visible)
    first = selected - visible + 1;

  for(int i = first; i < int(rows.size()) && i < first + visible; i++) {
    auto &row = rows[i];
    Rect r(0, top + (i - first) * row_h, w, row_h);
    bool is_selected = i == selected;

    if(is_selected) {
      draw_selection(Rect(r.x + 2, r.y, r.w - 4, r.h - 1));
    }

    Rect inner(r.x + 7, r.y, r.w - 12, r.h - 1);
    const int value_w = row.bar >= 0 ? 40 : screen.measure_text(row.value, minimal_font).w;

    screen.pen = row.dim && !is_selected ? theme::dim_text : theme::text;
    screen.text(fit_text(row.label, inner.w - value_w - 4), minimal_font, inner, true, TextAlign::center_left);

    if(row.bar >= 0)
      draw_bar(Rect(inner.x + inner.w - 40, inner.y + inner.h / 2 - 2, 40, 5), row.bar, row.bar_max, is_selected);
    else if(!row.value.empty()) {
      screen.pen = is_selected ? theme::text : theme::dim_text;
      screen.text(row.value, minimal_font, inner, true, TextAlign::center_right);
    }
  }
}

void draw_dialog(const std::string &message, const char *yes_label, const char *no_label) {
  const int w = screen.bounds.w, h = screen.bounds.h;
  const int margin = 8;
  const int text_w = w - margin * 2 - 10;

  auto wrapped = screen.wrap_text(message, text_w, minimal_font);
  int lines = 1;
  for(auto c : wrapped)
    if(c == '\n')
      lines++;

  Rect box(margin, 0, w - margin * 2, lines * 9 + 28);
  box.y = (h - box.h) / 2;

  screen.pen = theme::shade;
  screen.rectangle(Rect(0, 0, w, h));

  draw_panel(box);

  screen.pen = theme::text;
  screen.text(wrapped, minimal_font, Rect(box.x + 5, box.y + 7, text_w, lines * 9), true, TextAlign::top_center);

  if(no_label)
    draw_hints({{'A', yes_label}, {'B', no_label}}, box.y + box.h - 13);
  else
    draw_hints({{'A', yes_label}}, box.y + box.h - 13);
}

std::string format_size(uint32_t bytes) {
  char buf[16];
  if(bytes >= 1024 * 1024)
    snprintf(buf, sizeof(buf), "%u.%u MB", unsigned(bytes / (1024 * 1024)), unsigned(bytes % (1024 * 1024) * 10 / (1024 * 1024)));
  else
    snprintf(buf, sizeof(buf), "%u KB", unsigned((bytes + 1023) / 1024));
  return buf;
}
