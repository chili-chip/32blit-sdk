#pragma once

#include <initializer_list>
#include <string>

#include "engine/engine.hpp"
#include "graphics/font.hpp"

// the console's look, shared by the home screen and the system screens in the loader
// chili colours: hot reds and oranges on a deep roasted background
namespace theme {
  const blit::Pen background_top(52, 10, 16);
  const blit::Pen background_bottom(14, 4, 8);
  const blit::Pen panel(70, 16, 22);
  const blit::Pen chili(232, 32, 44);
  const blit::Pen hot(255, 110, 20);
  const blit::Pen pepper(255, 198, 30);
  const blit::Pen stem(90, 200, 60);
  const blit::Pen text(255, 255, 255);
  const blit::Pen dim_text(240, 170, 150);
  const blit::Pen faint_text(170, 100, 96);
  const blit::Pen bar_back(90, 26, 32);
  const blit::Pen shade(10, 2, 4, 150);
  const blit::Pen good(110, 220, 110);
}

constexpr int status_bar_h = 12;

blit::Pen mix(const blit::Pen &a, const blit::Pen &b, int t, int max);

// the full-screen background gradient
void draw_background();

// rectangles with the corners cut off, filled with a left to right gradient
void draw_round_rect(const blit::Rect &r, const blit::Pen &left, const blit::Pen &right);
void draw_round_rect(const blit::Rect &r, const blit::Pen &colour);

// battery and USB on the right, the ChiliChip wordmark or a title on the left
void draw_status_bar(const char *title = nullptr);

// a dark box with a hot outline, for dialogs and menus over other screens
void draw_panel(const blit::Rect &r);

// the highlight behind the selected row of a list
void draw_selection(const blit::Rect &r);

// "A Play  Y Info" with the letters in coloured buttons, centred at y
struct ButtonHint {
  char button;
  const char *label;
};
void draw_hints(std::initializer_list<ButtonHint> hints, int y);

// shorten text with ".." until it fits in max_w pixels
std::string fit_text(const std::string &text, int max_w, const blit::Font &font = blit::minimal_font);

// on_highlight draws it to stand out on a selected row
void draw_bar(const blit::Rect &r, int value, int max, bool on_highlight = false);
