#pragma once

#include <string>
#include <utility>
#include <vector>

#include "32blit.hpp"

#include "theme.hpp"

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

// a centred box with a message and button hints, for confirmations and errors
// no B hint without a no_label
void draw_dialog(const std::string &message, const char *yes_label, const char *no_label);

std::string format_size(uint32_t bytes);
