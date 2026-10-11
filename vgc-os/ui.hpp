#pragma once

#include <string>
#include <utility>
#include <vector>

#include "32blit.hpp"

namespace theme {
  const blit::Pen background(16, 16, 24);
  const blit::Pen panel(32, 34, 52);
  const blit::Pen highlight(50, 80, 150);
  const blit::Pen text(255, 255, 255);
  const blit::Pen dim_text(150, 150, 170);
  const blit::Pen accent(240, 90, 60);
  const blit::Pen good(110, 220, 110);
  const blit::Pen bad(255, 110, 90);
  const blit::Pen bar_back(70, 70, 90);
}

constexpr int status_bar_h = 12;

// battery and USB on the right, an optional title on the left
void draw_status_bar(const char *title = nullptr);

// shorten text with ".." until it fits in max_w pixels
std::string fit_text(const std::string &text, int max_w);

void draw_bar(const blit::Rect &r, int value, int max);

// buttons pressed this frame, with d-pad auto-repeat while held
uint32_t get_pressed(uint32_t time);

struct ListRow {
  ListRow() = default;
  ListRow(std::string label, std::string value = "") : label(std::move(label)), value(std::move(value)) {}

  std::string label;
  std::string value;   // right aligned
  int bar = -1;        // draw a bar instead of the value when >= 0
  int bar_max = 100;
  bool dim = false;
};

// full-screen list under the status bar, keeps the selected row in view
void draw_list(const char *title, const std::vector<ListRow> &rows, int selected);

// a centred box with a message and a hint line, for confirmations and errors
void draw_dialog(const std::string &message, const char *hint);

std::string format_size(uint32_t bytes);
