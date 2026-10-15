// Text: built-in fonts, alignment, measuring and wrapping.
#include "32blit.hpp"

using namespace blit;

const std::string story =
  "The chili pepper is the fruit of plants in the genus Capsicum. "
  "This text is wrapped to fit the box below, breaking between words.";

void init() {
  set_screen_mode(ScreenMode::hires);
}

void update(uint32_t time) {
}

void render(uint32_t time) {
  screen.pen = Pen(20, 20, 30);
  screen.clear();

  // the three built-in fonts
  screen.pen = Pen(255, 255, 255);
  screen.text("minimal_font", minimal_font, Point(2, 2));
  screen.text("fat_font", fat_font, Point(2, 12));
  screen.text("outline_font", outline_font, Point(2, 22));

  // variable = false uses the fixed character width, like a terminal
  screen.pen = Pen(250, 210, 60);
  screen.text("fixed", minimal_font, Point(126, 2), false, TextAlign::top_right);

  // alignment inside a rectangle
  Rect box(2, 34, 124, 30);
  screen.pen = Pen(60, 60, 80);
  screen.rectangle(box);
  screen.pen = Pen(255, 255, 255);
  screen.text("top left", minimal_font, box, true, TextAlign::top_left);
  screen.text("centre", minimal_font, box, true, TextAlign::center_center);
  screen.text("bottom right", minimal_font, box, true, TextAlign::bottom_right);

  // measure text to draw a background exactly behind it
  std::string label = "measured";
  Size size = screen.measure_text(label, minimal_font);
  screen.pen = Pen(214, 40, 40);
  screen.rectangle(Rect(Point(80, 18), size));
  screen.pen = Pen(255, 255, 255);
  screen.text(label, minimal_font, Point(80, 18));

  // wrap long text to a width, then draw it as several lines
  Rect para(2, 68, 124, 58);
  std::string wrapped = screen.wrap_text(story, para.w, minimal_font);
  screen.pen = Pen(200, 220, 255);
  screen.text(wrapped, minimal_font, para);
}
