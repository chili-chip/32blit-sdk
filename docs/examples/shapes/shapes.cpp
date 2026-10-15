// Shapes: every drawing primitive on one screen.
#include "32blit.hpp"

using namespace blit;

void init() {
  set_screen_mode(ScreenMode::hires);
}

void update(uint32_t time) {
}

void render(uint32_t time) {
  screen.pen = Pen(20, 20, 30);
  screen.clear();

  // filled rectangle
  screen.pen = Pen(214, 40, 40);
  screen.rectangle(Rect(8, 8, 32, 20));

  // circle, given a centre and a radius
  screen.pen = Pen(70, 170, 60);
  screen.circle(Point(64, 18), 11);

  // triangle
  screen.pen = Pen(70, 120, 220);
  screen.triangle(Point(92, 28), Point(104, 6), Point(118, 28));

  // lines, and fast horizontal and vertical spans
  screen.pen = Pen(250, 210, 60);
  screen.line(Point(8, 40), Point(120, 56));
  screen.h_span(Point(8, 62), 112);
  screen.v_span(Point(124, 36), 30);

  // polygon from a list of points
  screen.pen = Pen(230, 130, 40);
  screen.polygon({Point(10, 76), Point(40, 70), Point(50, 96), Point(24, 104), Point(6, 92)});

  // single pixels: a small starfield that drifts left
  screen.pen = Pen(255, 255, 255);
  for(int i = 0; i < 24; i++) {
    int x = (i * 37 + 128 - (time / 40) % 128) % 128;
    screen.pixel(Point(x, 108 + (i * 7) % 18));
  }

  // alpha: a translucent rectangle over the shapes underneath
  screen.pen = Pen(255, 255, 255, 96);
  screen.rectangle(Rect(60, 70, 60, 34));

  // global alpha fades everything drawn after it
  screen.alpha = 128;
  screen.pen = Pen(250, 170, 180);
  screen.circle(Point(90, 87), 14);
  screen.alpha = 255;
}
