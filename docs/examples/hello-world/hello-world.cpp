// Hello World: the smallest complete game.
//
// Every game provides three functions: init() runs once at start-up,
// update() runs 100 times a second to move the game on, and render()
// runs once per frame to draw it.
#include "32blit.hpp"

using namespace blit;

void init() {
  // 128x128 pixels, the full VGC Zero screen
  set_screen_mode(ScreenMode::hires);
}

void update(uint32_t time) {
  // nothing to update yet
}

void render(uint32_t time) {
  // fade the background through the colour wheel, one turn every 10 seconds
  float hue = (time % 10000) / 10000.0f;
  screen.pen = hsv_to_rgba(hue, 0.6f, 0.4f);
  screen.clear();

  screen.pen = Pen(255, 255, 255);
  screen.text("Hello, VGC Zero!", minimal_font, Point(64, 60), true, TextAlign::center_center);

  screen.pen = Pen(200, 200, 200);
  screen.text(std::to_string(time / 1000) + " s", minimal_font, Point(64, 72), true, TextAlign::center_center);
}
