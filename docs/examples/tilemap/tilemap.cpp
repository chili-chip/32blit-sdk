// Tile map: build a level from a spritesheet and scroll around it.
//
// A TileMap is a grid of sprite indices. Its width and height (in tiles)
// must be powers of two, and its spritesheet must be 128 pixels wide
// (16 sprites per row). Levels drawn in Tiled can be packed with
// assets.yml and loaded with TileMap::load_tmx() instead.
#include <cstring>

#include "32blit.hpp"
#include "assets.hpp"

using namespace blit;

// sprite indices in examples/shared/sprites.png
enum Tile : uint8_t {
  COIN = 4, GRASS = 8, DIRT = 9, BRICK = 10, WATER = 11, SKY = 12, FLOWER = 13, CLOUD = 15
};

constexpr int map_w = 32, map_h = 16; // in tiles, both powers of two

// The level as text, one character per tile. Easier to edit than numbers.
const char *level[map_h] = {
  "................................",
  "................................",
  "....c...........c.........c.....",
  "...........c....................",
  "................................",
  "..............$$$...............",
  ".............bbbbb.......$$.....",
  "........................bbbb....",
  ".....$$.........................",
  "....bbbb..............f.........",
  ".........f.....f....ggggggg.....",
  "gggggggggggg..ggggggddddddd.gggg",
  "ddddddddddddwwddddddddddddddwddd",
  "ddddddddddddwwddddddddddddddwddd",
  "dddddddddddddddddddddddddddddddd",
  "dddddddddddddddddddddddddddddddd",
};

uint8_t tiles[map_w * map_h];
TileMap *map = nullptr;
Surface *sheet = nullptr;

Vec2 camera(0, 0);

void init() {
  set_screen_mode(ScreenMode::hires);

  sheet = Surface::load(asset_sprites);
  screen.sprites = sheet;

  // turn the text into tile indices
  for(int y = 0; y < map_h; y++) {
    for(int x = 0; x < map_w; x++) {
      uint8_t t = SKY;
      switch(level[y][x]) {
        case 'g': t = GRASS; break;
        case 'd': t = DIRT; break;
        case 'b': t = BRICK; break;
        case 'w': t = WATER; break;
        case 'f': t = FLOWER; break;
        case 'c': t = CLOUD; break;
        case '$': t = COIN; break;
      }
      tiles[x + y * map_w] = t;
    }
  }

  // no per-tile transforms (nullptr), map size in tiles, and the spritesheet
  map = new TileMap(tiles, nullptr, Size(map_w, map_h), sheet);

  // SKY tiles are skipped when drawing, so the background shows through
  map->empty_tile_id = SKY;

  // wrap around horizontally when the camera goes past the edge
  map->repeat_mode = TileMap::REPEAT;
}

void update(uint32_t time) {
  if(buttons & Button::DPAD_LEFT)  camera.x -= 1.0f;
  if(buttons & Button::DPAD_RIGHT) camera.x += 1.0f;
  if(buttons & Button::DPAD_UP)    camera.y -= 1.0f;
  if(buttons & Button::DPAD_DOWN)  camera.y += 1.0f;

  camera.y = std::max(-32.0f, std::min(camera.y, float(map_h * 8 - 128)));

  // A collects the coin under the centre of the screen, if there is one
  if(buttons.pressed & Button::A) {
    Point centre = Point(camera + Vec2(64, 64)) / 8; // pixels to tiles
    int32_t index = map->offset(centre);
    if(index >= 0 && tiles[index] == COIN)
      tiles[index] = SKY;
  }
}

void render(uint32_t time) {
  // sky gradient behind the map
  for(int y = 0; y < 128; y++) {
    screen.pen = Pen(90 + y / 2, 150 + y / 3, 230);
    screen.h_span(Point(0, y), 128);
  }

  // the map's transform turns screen positions into map positions (in pixels),
  // so translating by the camera position scrolls the map
  map->transform = Mat3::translation(camera);
  map->draw(&screen, Rect(0, 0, 128, 128));

  // crosshair in the centre
  screen.pen = Pen(255, 255, 255);
  screen.h_span(Point(60, 64), 9);
  screen.v_span(Point(64, 60), 9);

  Point centre = Point(camera + Vec2(64, 64)) / 8;
  screen.pen = Pen(0, 0, 0, 160);
  screen.rectangle(Rect(0, 0, 128, 10));
  screen.pen = Pen(255, 255, 255);
  screen.text("tile " + std::to_string(centre.x) + "," + std::to_string(centre.y) +
              " = " + std::to_string(map->tile_at(centre)) + "  A: take coin", minimal_font, Point(2, 1));
}
