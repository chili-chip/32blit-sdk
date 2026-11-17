#include <algorithm>

#include "layout.hpp"

int text_width(std::string_view text, const blit::Font &font) {
  int w = 0;
  for(char c : text) {
    uint8_t i = c & 0x7f;
    w += font.char_w_variable[i < ' ' ? 0 : i - ' '];
  }
  return w;
}

std::string fit_text(std::string_view text, int max_w, const blit::Font &font) {
  if(text_width(text, font) <= max_w)
    return std::string(text);

  const int dots_w = text_width("..", font);
  while(!text.empty() && text_width(text, font) + dots_w > max_w)
    text.remove_suffix(1);

  // "Space.." not "Space .."
  while(!text.empty() && text.back() == ' ')
    text.remove_suffix(1);

  return std::string(text) + "..";
}

int scroll_offset(int text_w, int box_w, uint32_t elapsed_ms) {
  const int dist = text_w - box_w;
  if(dist <= 0)
    return 0;

  const uint32_t wait = 1000, speed = 30; // px/s
  const uint32_t run = dist * 1000 / speed;
  const uint32_t t = elapsed_ms % (wait + run + wait);

  if(t < wait)
    return -1;

  return std::min<int>(dist, (t - wait) * speed / 1000);
}

int ScrollingText::offset(std::string_view text, int text_w, int box_w, uint32_t now) {
  if(!started || text != this->text) {
    this->text = text;
    restart(now);
  }
  return scroll_offset(text_w, box_w, now - start);
}

void ScrollingText::restart(uint32_t now) {
  start = now;
  started = true;
}

int list_first_row(int selected, int count, int visible) {
  return std::clamp(selected - visible + 1, 0, std::max(0, count - visible));
}

// rounded a * b / c, for positive values
static int scale(int a, int b, int c) {
  return (a * b * 2 + c) / (c * 2);
}

Span scroll_thumb(int first, int count, int visible, int track_h) {
  if(count <= visible)
    return {0, 0};

  int size = std::max(4, scale(track_h, visible, count));
  return {scale(track_h - size, first, count - visible), size};
}

Span position_bar(int selected, int count, int width) {
  if(count <= 1)
    return {0, width};

  int size = std::max(8, width / count);
  return {scale(width - size, selected, count - 1), size};
}

int slide_offset(int from, uint32_t elapsed_ms) {
  const int duration = 150;
  if(elapsed_ms >= uint32_t(duration))
    return 0;

  return from * (duration - int(elapsed_ms)) / duration;
}

std::vector<std::string> wrap_words(std::string_view text, int max_w, int scale, const blit::Font &font) {
  std::vector<std::string> lines;
  std::string line;

  while(!text.empty()) {
    auto end = text.find(' ');
    auto word = text.substr(0, end);
    text = end == std::string_view::npos ? std::string_view() : text.substr(end + 1);
    if(word.empty())
      continue;

    std::string joined = line.empty() ? std::string(word) : line + " " + std::string(word);
    if(line.empty() || text_width(joined, font) * scale <= max_w)
      line = joined;
    else {
      lines.push_back(line);
      line = word;
    }
  }

  if(!line.empty())
    lines.push_back(line);

  return lines;
}
