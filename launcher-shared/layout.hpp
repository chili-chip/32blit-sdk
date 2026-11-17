#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

#include "graphics/font.hpp"

// Layout maths for the console OS look. No drawing here, so it is tested on the host
// (layout_test.cpp). theme.hpp draws with it.

// width of one line of text in pixels, with variable width characters
int text_width(std::string_view text, const blit::Font &font = blit::minimal_font);

// the text cut with ".." until it fits in max_w pixels
std::string fit_text(std::string_view text, int max_w, const blit::Font &font = blit::minimal_font);

// words on lines of at most max_w pixels, for text drawn at `scale` times its size.
// A word longer than a line gets a line of its own.
std::vector<std::string> wrap_words(std::string_view text, int max_w, int scale = 1, const blit::Font &font = blit::minimal_font);

// Text that doesn't fit its box is shown cut with "..", then after 1 s it scrolls to its end
// at 30 px/s, waits 1 s and starts again.
// How far to move the text left after elapsed_ms, or -1 while it shows cut.
int scroll_offset(int text_w, int box_w, uint32_t elapsed_ms);

// scroll_offset() with the start time: starts again when the text changes
struct ScrollingText {
  int offset(std::string_view text, int text_w, int box_w, uint32_t now);
  void restart(uint32_t now);

  std::string text;
  uint32_t start = 0;
  bool started = false;
};

// the first row to show so that the selected one is in view, on the last line when scrolled
int list_first_row(int selected, int count, int visible);

struct Span {
  int pos, size;
};

// the scroll bar thumb in a track of track_h pixels, size 0 when every row fits
Span scroll_thumb(int first, int count, int visible, int track_h);

// the home screen's position bar: where the selected one of count tiles is across width pixels
Span position_bar(int selected, int count, int width);

// the home screen slides a cover in 150 ms: the offset after elapsed_ms, from `from` pixels to 0
int slide_offset(int from, uint32_t elapsed_ms);
