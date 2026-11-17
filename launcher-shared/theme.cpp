#include <algorithm>

#include "theme.hpp"

#include "engine/system.hpp"

using namespace blit;

// pixel art: '#' is drawn
static void draw_pattern(Point p, std::initializer_list<const char *> rows, const Pen &colour) {
  screen.pen = colour;
  int y = p.y;
  for(auto row : rows) {
    for(int x = 0; row[x]; x++) {
      if(row[x] == '#')
        screen.pixel(Point(p.x + x, y));
    }
    y++;
  }
}

static void draw_outline(const Rect &r) {
  screen.h_span(Point(r.x, r.y), r.w);
  screen.h_span(Point(r.x, r.y + r.h - 1), r.w);
  screen.v_span(Point(r.x, r.y), r.h);
  screen.v_span(Point(r.x + r.w - 1, r.y), r.h);
}

void draw_text_2x(std::string_view text, Point p) {
  const Font &font = minimal_font;
  const int height_bytes = (font.char_h + 7) / 8;

  for(char c : text) {
    uint8_t i = c & 0x7f;
    i = i < ' ' ? 0 : i - ' ';
    const uint8_t *glyph = &font.data[i * font.char_w * height_bytes];

    for(int x = 0; x < font.char_w; x++) {
      for(int y = 0; y < font.char_h; y++) {
        if(glyph[x * height_bytes + y / 8] & (1 << (y & 7)))
          screen.rectangle(Rect(p.x + x * 2, p.y + y * 2, 2, 2));
      }
    }
    p.x += font.char_w_variable[i] * 2;
  }
}

void draw_background() {
  screen.pen = theme::bg;
  screen.clear();
}

static void draw_battery(Point p, int level) {
  // 10x6 outline with a nub on the right, so the bright area stays small on the OLED
  screen.pen = theme::muted;
  draw_outline(Rect(p.x, p.y, 10, 6));
  screen.rectangle(Rect(p.x + 10, p.y + 2, 1, 2));

  if(level < 0)
    return;

  screen.pen = level <= 15 ? theme::warning : theme::success;
  screen.rectangle(Rect(p.x + 2, p.y + 2, std::max(1, (std::min(level, 100) * 6 + 50) / 100), 2));
}

void draw_status_bar(const char *title) {
  const int w = screen.bounds.w;

  int x = w - 16;
  draw_battery(Point(x, 3), get_battery_level());

  if(is_usb_connected()) {
    x -= 10;
    draw_pattern(Point(x, 3), {".#..#.", ".#..#.", "######", "######", ".####.", "..##.."}, theme::muted);
  }

  if(title) {
    screen.pen = theme::muted;
    screen.text(fit_text(title, x - margin * 2), minimal_font, Point(margin, 2));
  }

  screen.pen = theme::line;
  screen.h_span(Point(0, status_bar_h), w);
}

void draw_hints(const char *left, const char *right) {
  const int w = screen.bounds.w, y = screen.bounds.h - hint_h;

  screen.pen = theme::muted;
  if(left && *left)
    screen.text(left, minimal_font, Point(margin, y));
  if(right && *right)
    screen.text(right, minimal_font, Point(w - margin - text_width(right), y));
}

void draw_scrolling_text(ScrollingText &state, const std::string &text, const Rect &box) {
  const int text_w = text_width(text);
  int offset = state.offset(text, text_w, box.w, now());

  if(offset < 0) {
    screen.text(fit_text(text, box.w), minimal_font, Point(box.x, box.y));
    return;
  }

  auto old_clip = screen.clip;
  screen.clip = old_clip.intersection(Rect(box.x, box.y, box.w, std::max<int32_t>(box.h, 9)));
  screen.text(text, minimal_font, Rect(box.x - offset, box.y, text_w, 9));
  screen.clip = old_clip;
}

void draw_bar(const Rect &r, int value, int max, bool on_accent) {
  screen.pen = on_accent ? theme::accent_ink : theme::line;
  screen.rectangle(r);

  int fill = max > 0 ? int((int64_t(r.w) * std::clamp(value, 0, max) * 2 + max) / (int64_t(max) * 2)) : 0;
  if(fill <= 0)
    return;

  screen.pen = on_accent ? theme::on_accent : theme::text;
  screen.rectangle(Rect(r.x, r.y, fill, r.h));
}

void draw_row(const Rect &r, const ListRow &row, bool selected) {
  // the selected row's label, only one is shown at a time
  static ScrollingText selected_label;

  const int ty = r.y + (r.h - 8) / 2 - (selected ? 1 : 0);
  const int right = r.x + r.w - 6;
  const int bar_w = r.w >= 120 ? 40 : 28;

  if(selected) {
    screen.pen = theme::accent;
    screen.rectangle(r);
    int strip = r.h >= row_h ? 3 : 2;
    screen.pen = theme::accent_shade;
    screen.rectangle(Rect(r.x, r.y + r.h - strip, r.w, strip));
    screen.pen = theme::on_accent;
    screen.text(">", minimal_font, Point(r.x + 4, ty));
  }

  int label_w = r.w - 20;
  if(row.bar >= 0)
    label_w -= bar_w + 4;
  else if(!row.value.empty())
    label_w -= text_width(row.value) + 4;

  screen.pen = selected ? theme::on_accent : row.dim ? theme::slate : theme::text;
  if(selected)
    draw_scrolling_text(selected_label, row.label, Rect(r.x + 12, ty, label_w, 9));
  else
    screen.text(fit_text(row.label, label_w), minimal_font, Point(r.x + 12, ty));

  if(row.bar >= 0)
    draw_bar(Rect(right - bar_w, ty + 2, bar_w, 3), row.bar, row.bar_max, selected);
  else if(!row.value.empty()) {
    screen.pen = selected ? theme::on_accent : theme::muted;
    screen.text(row.value, minimal_font, Point(right - text_width(row.value), ty));
  }
}

void draw_list(const char *title, const std::vector<ListRow> &rows, int selected, const char *hint_left, const char *hint_right) {
  const int w = screen.bounds.w;
  const int count = rows.size();
  const int first = list_first_row(selected, count, list_rows);

  draw_background();
  draw_status_bar(title);

  for(int i = first; i < count && i < first + list_rows; i++)
    draw_row(Rect(0, list_top + (i - first) * row_h, w, row_h), rows[i], i == selected);

  auto thumb = scroll_thumb(first, count, list_rows, list_rows * row_h);
  if(thumb.size) {
    screen.pen = theme::line;
    screen.rectangle(Rect(w - 2, list_top, 2, list_rows * row_h));
    screen.pen = theme::muted;
    screen.rectangle(Rect(w - 2, list_top + thumb.pos, 2, thumb.size));
  }

  draw_hints(hint_left, hint_right);
}

void draw_panel(const Rect &r) {
  const int cut = 6;

  screen.pen = theme::shadow;
  screen.rectangle(Rect(r.x + 4, r.y + 4, r.w, r.h));

  screen.pen = theme::surface;
  for(int y = 0; y < r.h; y++) {
    int inset = std::max(0, cut - y);
    screen.h_span(Point(r.x, r.y + y), r.w - inset);
  }

  screen.pen = theme::line;
  screen.h_span(Point(r.x, r.y), r.w - cut);
  screen.h_span(Point(r.x, r.y + r.h - 1), r.w);
  screen.v_span(Point(r.x, r.y), r.h);
  screen.v_span(Point(r.x + r.w - 1, r.y + cut), r.h - cut);
  for(int i = 0; i < cut; i++)
    screen.pixel(Point(r.x + r.w - cut - 1 + i, r.y + i));
}

int draw_button(Point p, char button, const char *label) {
  std::string text = std::string(1, button) + " " + label;
  const int w = text_width(text) + 10, h = 12;

  if(button == 'A') {
    screen.pen = theme::accent;
    screen.rectangle(Rect(p.x, p.y, w, h));
    screen.pen = theme::accent_shade;
    screen.rectangle(Rect(p.x, p.y + h - 2, w, 2));
    screen.pen = theme::on_accent;
  } else {
    screen.pen = theme::muted;
    draw_outline(Rect(p.x, p.y, w, h));
    screen.pen = theme::text;
  }

  screen.text(text, minimal_font, Point(p.x + 5, p.y + 2));
  return w;
}

void draw_dialog(const std::string &title, const std::string &detail, const char *yes_label, const char *no_label) {
  const int sw = screen.bounds.w, sh = screen.bounds.h;
  const int w = std::min(112, sw - 16), text_w = w - 16;
  const int max_lines = 3;

  // up to three lines of detail, the last one cut when there is more
  auto lines = wrap_words(detail, text_w);
  if(int(lines.size()) > max_lines) {
    lines[max_lines - 1] = fit_text(lines[max_lines - 1] + " ...", text_width(lines[max_lines - 1]));
    lines.resize(max_lines);
  }

  const int buttons_y = 20 + int(lines.size()) * 9 + (lines.empty() ? 0 : 4);
  const int h = buttons_y + 20;
  const Rect box((sw - w) / 2, (sh - h - 4) / 2, w, h);

  screen.pen = theme::dim;
  screen.rectangle(Rect(0, 0, sw, sh));

  draw_panel(box);

  screen.pen = theme::text;
  screen.text(fit_text(title, text_w - 2), minimal_font, Point(box.x + 8, box.y + 8));

  screen.pen = theme::muted;
  for(size_t i = 0; i < lines.size(); i++)
    screen.text(lines[i], minimal_font, Point(box.x + 8, box.y + 20 + int(i) * 9));

  int x = box.x + 8;
  x += draw_button(Point(x, box.y + buttons_y), 'A', yes_label) + 4;
  if(no_label)
    draw_button(Point(x, box.y + buttons_y), 'B', no_label);
}
