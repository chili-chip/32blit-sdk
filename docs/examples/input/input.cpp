// Input: move a square with the d-pad and show which buttons are held.
#include "32blit.hpp"

using namespace blit;

Vec2 position(60, 60);
Pen colour(214, 40, 40);
int presses = 0;

void init() {
  set_screen_mode(ScreenMode::hires);
}

void update(uint32_t time) {
  // update() runs every 10 ms, so 0.5 pixels per update is 50 pixels a second
  const float speed = 0.5f;

  // held buttons: test the bits of `buttons`
  if(buttons & Button::DPAD_LEFT)  position.x -= speed;
  if(buttons & Button::DPAD_RIGHT) position.x += speed;
  if(buttons & Button::DPAD_UP)    position.y -= speed;
  if(buttons & Button::DPAD_DOWN)  position.y += speed;

  // keep the square on screen
  position.x = std::max(0.0f, std::min(position.x, 120.0f));
  position.y = std::max(12.0f, std::min(position.y, 120.0f));

  // buttons.pressed only holds buttons that went down since the last update,
  // so this runs once per press however long the button is held
  if(buttons.pressed & Button::A) {
    colour = hsv_to_rgba((blit::random() % 360) / 360.0f, 0.8f, 1.0f);
    presses++;
  }

  // buttons.released is the same for buttons that were let go
  if(buttons.released & Button::B)
    position = Vec2(60, 60);
}

void render(uint32_t time) {
  screen.pen = Pen(20, 20, 30);
  screen.clear();

  screen.pen = colour;
  screen.rectangle(Rect(Point(position), Size(8, 8)));

  // list the held buttons along the top
  static const struct { Button button; const char *name; } names[] = {
    {Button::A, "A"}, {Button::B, "B"}, {Button::X, "X"}, {Button::Y, "Y"},
    {Button::DPAD_UP, "U"}, {Button::DPAD_DOWN, "D"}, {Button::DPAD_LEFT, "L"}, {Button::DPAD_RIGHT, "R"},
    {Button::MENU, "MENU"},
  };

  std::string held;
  for(auto &n : names) {
    if(pressed(n.button))
      held += std::string(n.name) + " ";
  }

  screen.pen = Pen(255, 255, 255);
  screen.text("Held: " + held, minimal_font, Point(2, 2));
  screen.text("A presses: " + std::to_string(presses), minimal_font, Point(2, 118));
}
