// Sprites: draw from a spritesheet, animate, flip, rotate and scale.
//
// The sheet is assets/sprites.png, a grid of 8x8 sprites packed into the
// game by assets.yml. Sprite 0 is the top-left one, counting along each row.
#include "32blit.hpp"
#include "assets.hpp"

using namespace blit;

// sprite indices in the sheet
enum Sprite : uint16_t {
  CHILI = 0, PLAYER_WALK_1 = 1, PLAYER_WALK_2 = 2, HEART = 3, COIN = 4, BOMB = 5, STAR = 6
};

void init() {
  set_screen_mode(ScreenMode::hires);

  // load the packed image into a surface and make it the active spritesheet
  screen.sprites = Surface::load(asset_sprites);
}

void update(uint32_t time) {
}

void render(uint32_t time) {
  screen.pen = Pen(40, 60, 90);
  screen.clear();

  screen.pen = Pen(255, 255, 255);
  screen.text("Index, Point, Rect", minimal_font, Point(4, 2));

  // three ways to pick a sprite: by index...
  screen.sprite(CHILI, Point(8, 14));
  // ...by its column and row in 8x8 units...
  screen.sprite(Point(3, 0), Point(24, 14));
  // ...or a block of several sprites as one (here 2x1 sprites wide)
  screen.sprite(Rect(1, 0, 2, 1), Point(40, 14));

  screen.text("Animation", minimal_font, Point(4, 28));
  // swap between two frames every 200 ms
  uint16_t frame = (time / 200) % 2 ? PLAYER_WALK_1 : PLAYER_WALK_2;
  screen.sprite(frame, Point(8, 40));

  // walk across the screen, flipping to face the direction of travel
  int t = (time / 16) % 200;
  int x = t < 100 ? t : 200 - t;
  screen.sprite(frame, Point(20 + x, 40), t < 100 ? SpriteTransform::NONE : SpriteTransform::HORIZONTAL);

  screen.text("Transforms", minimal_font, Point(4, 54));
  const SpriteTransform transforms[] = {
    SpriteTransform::NONE, SpriteTransform::HORIZONTAL, SpriteTransform::VERTICAL,
    SpriteTransform::R90, SpriteTransform::R180, SpriteTransform::R270
  };
  for(int i = 0; i < 6; i++)
    screen.sprite(CHILI, Point(8 + i * 14, 66), transforms[i]);

  screen.text("Scaling", minimal_font, Point(4, 80));
  // scale around the sprite's centre (origin 4,4) so it grows in place
  float scale = 1.0f + 1.5f * (0.5f + 0.5f * sinf(time / 300.0f));
  screen.sprite(COIN, Point(20, 106), Point(4, 4), scale);
  screen.sprite(HEART, Point(52, 106), Point(4, 4), 3.0f);
  screen.sprite(BOMB, Point(84, 106), Point(4, 4), Vec2(4.0f, 2.0f));
  screen.sprite(STAR, Point(112, 106), Point(4, 4), 2.0f);
}
