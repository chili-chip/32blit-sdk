# Tile maps and Mode 7

`graphics/tilemap.hpp`, `graphics/mode7.hpp`, `types/map.hpp`

`TileMap` draws a grid of 8×8 tiles, with scrolling, scaling, rotation and per-scanline effects. The [tile map tutorial](../tutorials/tilemaps.md) builds a scrolling level step by step.

## `TileMap`

```cpp
struct TileMap {
  Size bounds;                         // size in tiles
  uint8_t *tiles;                      // bounds.w * bounds.h tile ids, row by row
  uint8_t *transforms;                 // same size, or nullptr
  Surface *sprites;                    // spritesheet, 128 pixels wide
  Mat3 transform = Mat3::identity();   // screen position -> map position, in pixels

  enum { NONE, REPEAT, DEFAULT_FILL, CLAMP_TO_EDGE } repeat_mode = NONE;
  uint8_t default_tile_id;
  int empty_tile_id = -1;

  enum LoadFlags { copy_tiles = 1, copy_transforms = 2 };

  TileMap(uint8_t *tiles, uint8_t *transforms, Size bounds, Surface *sprites);
  static TileMap *load_tmx(const uint8_t *asset, Surface *sprites, int layer = 0,
                           int flags = copy_tiles | copy_transforms);

  int32_t offset(const Point &p);
  int32_t offset(int16_t x, int16_t y);
  uint8_t tile_at(const Point &p);
  uint8_t transform_at(const Point &p);

  void draw(Surface *dest, Rect viewport,
            std::function<Mat3(uint8_t)> scanline_callback = nullptr);
  void texture_span(Surface *dest, Point s, unsigned int c, Vec2 swc, Vec2 ewc);
};
```

### Requirements

* `bounds` must be a power of two in each direction (8, 16, 32, 64...). Positions are wrapped with a bit mask, so other sizes draw the wrong tiles.
* Each tile is an 8×8 sprite. A tile id `n` is the sprite at column `n % 16`, row `n / 16` of `sprites`, so the sheet must be **128 pixels wide**, and a map can use 256 different tiles.
* The map keeps pointers to `tiles` and `transforms`; it doesn't copy them. Keep the arrays alive, and change tiles by writing to them.
* World positions are converted to 16.16 fixed point while drawing, so keep the visible area of the map within ±32 767 pixels.

### Fields

`transform`
:   Turns screen positions into map positions in pixels. For plain scrolling, `Mat3::translation(camera)`; combine it with `rotation()` and `scale()` for zooming and spinning (see [combining transforms](types.md#combining-transforms)). Scaling by 0.5 zooms *in*, because it's the map position for each screen pixel.

`repeat_mode`
:   What's drawn outside the map:

    | Mode | Outside the map |
    | ---- | --------------- |
    | `NONE` | nothing; the screen keeps what was drawn before |
    | `REPEAT` | the map repeats forever |
    | `CLAMP_TO_EDGE` | the nearest edge tile is stretched outwards |
    | `DEFAULT_FILL` | see the note below |

`empty_tile_id`
:   Tiles with this id aren't drawn, so anything drawn earlier (a background, another map) shows through. `-1`, the default, draws every tile.

`default_tile_id`
:   For `DEFAULT_FILL`.

!!! warning "`DEFAULT_FILL` uses an index"
    With `DEFAULT_FILL`, positions outside the map use the tile *stored at index* `default_tile_id` of the `tiles` array, not the tile id `default_tile_id`. To fill with tile id `t`, put `t` at that index, for example in a corner of the map that's always covered, or use `REPEAT` with a border of the fill tile.

### Drawing

`draw(dest, viewport)`
:   Draws the map into the `viewport` rectangle of `dest` (clipped to `dest->clip`). Each screen pixel shows the map pixel at `pixel * transform`. Tile pixels are drawn with `dest`'s pen blending, so `dest->alpha` and `dest->mask` apply and transparent sprite pixels are skipped.

`draw(dest, viewport, scanline_callback)`
:   The same, but calls `scanline_callback(y)` for each screen row and uses the returned matrix for that row instead of `transform`. That allows per-line effects: wobbling water, perspective floors, split screens.

```cpp
// wavy "underwater" effect
map->draw(&screen, Rect(0, 0, 128, 128), [](uint8_t y) {
  float wobble = sinf(y * 0.2f + now() * 0.005f) * 3.0f;
  return Mat3::translation(camera + Vec2(wobble, 0));
});
```

`texture_span(dest, s, c, swc, ewc)`
:   Draws one row of `c` pixels starting at screen position `s`, sampling the map along the line from world position `swc` to `ewc` (one pixel past the end). `draw()` uses it; call it yourself for custom renderers. It doesn't clip, so `s` and `c` must be inside `dest`.

### Reading tiles

`tile_at(p)`
:   The tile id at tile position `p` (pixels divided by 8). Outside the map it follows `repeat_mode`, and gives `0` with `NONE`.

`transform_at(p)`
:   The tile's transform bits, or `0` if there's no `transforms` array.

`offset(p)`
:   The index into `tiles` and `transforms` for a tile position, following `repeat_mode`, or `-1` outside the map with `NONE`.

```cpp
bool is_solid(Vec2 world) {
  Point t = Point(world) / 8;
  uint8_t id = map->tile_at(t);
  return id == BRICK || id == GRASS;
}
```

`Point(world) / 8` rounds towards zero, which is wrong for negative positions; keep the player inside the map or use `floorf(world.x / 8)` if they can go left of 0.

### Tile transforms

Each byte of `transforms` flips or rotates its tile:

| Bit | Effect |
| --- | ------ |
| `0b100` | flip horizontally |
| `0b010` | flip vertically |
| `0b001` | swap x and y (applied after the flips) |

Combine them for rotations: `0b101` is 90° clockwise, `0b011` is 90° anticlockwise, and `0b110` (both flips) is 180°. These bits are the same as Tiled's flip flags, so maps from Tiled need no conversion.

!!! note
    These bits are not the same as [`SpriteTransform`](sprites.md#spritetransform), where `HORIZONTAL` is `0b001` and `XYSWAP` is `0b100`. Don't pass `SpriteTransform` values in a map's `transforms` array.

### Loading Tiled maps

`load_tmx(asset, sprites, layer, flags)`
:   Creates a map from a Tiled `.tmx` file packed by the [asset pipeline](../guides/assets.md) with `output_struct: true`:

    ```yaml title="assets.yml"
    assets.cpp:
      assets/level.tmx:
        name: asset_level
        type: map/tiled
        output_struct: true
        empty_tile: 255
    ```

    ```cpp
    TileMap *level = TileMap::load_tmx(asset_level, sheet);           // layer 0
    TileMap *decor = TileMap::load_tmx(asset_level, sheet, 1);        // layer 1
    ```

    Returns `nullptr` if the data isn't a packed map, uses 16-bit tiles (more than 256 tile types), or its size isn't a power of two.

`layer`
:   Which layer to load, in the order of the layers' ids in Tiled. Load each layer into its own `TileMap` and draw them back to front.

`flags`
:   By default the tiles and transforms are copied into RAM, so you can change them. Pass `0` to read them straight from flash and save RAM, for maps that never change. Without `copy_transforms`, a map with no flipped tiles has `transforms == nullptr`.

`empty_tile_id`
:   Set from the map: cells left empty in Tiled get the `empty_tile` id from `assets.yml` (`0` if not set) and aren't drawn. Because the default is `0`, the first sprite in the sheet can't be used as a tile unless you set `empty_tile` to an id you don't use, such as `255`.

## Mode 7

```cpp
void mode7(Surface *dest, Surface *tiles, MapLayer *layer, float fov, float angle,
           Vec2 pos, float near, float far, Rect viewport);
Vec2  world_to_screen(Vec2 w, float fov, float angle, Vec2 pos, float near, float far, Rect viewport);
Vec2  screen_to_world(Vec2 s, float fov, float angle, Vec2 pos, float near, float far, Rect viewport);
```

A perspective floor in the style of the SNES "Mode 7", drawn from the older `Map`/`MapLayer` types below. It's experimental. Call `tiles->generate_mipmaps(depth)` first to blend distant rows smoothly; without mipmaps the sheet is drawn at full size. `viewport` can be anywhere on the screen, and `dest->pen` and `dest->alpha` are left as they were.

For new games, a `TileMap` with a `scanline_callback` gives the same effect with less overhead. Each row of the floor is a horizontal line of the map, further away and more zoomed out the closer it is to the horizon:

```cpp
Vec2 camera(128, 128);  // position on the map, in pixels
float heading = 0.0f;   // radians; 0 looks up the map

void render(uint32_t time) {
  // sky above the horizon at y = 32
  screen.pen = Pen(120, 180, 255);
  screen.rectangle(Rect(0, 0, 128, 32));

  map->draw(&screen, Rect(0, 32, 128, 96), [](uint8_t y) {
    float z = 16.0f / float(y - 31);               // scale of this row
    return Mat3::translation(camera)
         * Mat3::rotation(heading)
         * Mat3::scale(Vec2(z, z))
         * Mat3::translation(Vec2(-64.0f, -float(y) - 64.0f));
  });
}
```

Read the matrices from the right: the row's pixels are moved so its centre is 64 units in front of the origin, scaled by the row's distance, rotated to the camera's heading, and moved to the camera's position. Use `REPEAT` so the floor goes on forever.

`world_to_screen(w, ...)`
:   Where a world position appears on screen in `mode7()`'s view, for drawing sprites on the floor.

`screen_to_world(s, ...)`
:   The reverse: the world position under screen point `s`, for example to find the tile under the cursor.

## `Map` and `MapLayer`

```cpp
struct Map {
  Rect bounds;
  std::map<std::string, MapLayer> layers;
  std::vector<uint8_t> flags;           // one byte of flags per tile

  Map(Rect bounds);
  void add_layer(std::string name, std::vector<uint8_t> tiles);
  int32_t tile_index(Point p);          // -1 outside bounds
  uint8_t get_flags(Point p);
  bool has_flag(Point p, uint8_t f);
  void tiles_in_rect(Rect r, std::function<void(Point)> f);
};

struct MapLayer {
  Map *map;
  std::string name;
  std::vector<uint8_t> tiles;
  std::vector<uint8_t> transforms;

  void add_flags(uint8_t tile, uint8_t flags);
  void add_flags(std::vector<uint8_t> tiles, uint8_t flags);
  uint8_t tile_at(Point p);
  uint8_t transform_at(Point p);
};
```

An older, more general map type used by `mode7()`. Its useful part for any game is the **flags**: mark tile types with bits such as "solid" or "water", then look them up by position.

```cpp
enum { SOLID = 1, WATER = 2 };

Map world(Rect(0, 0, 32, 32));

void init() {
  world.add_layer("ground", level_tiles);            // a std::vector<uint8_t>
  world.layers["ground"].add_flags({BRICK, GRASS}, SOLID);
  world.layers["ground"].add_flags(WATER_TILE, WATER);
}

bool blocked(Point tile) {
  return world.has_flag(tile, SOLID);
}
```

`tiles_in_rect(r, f)`
:   Calls `f` with the tile position of every tile under the pixel rectangle `r`, clipped to the map. Handy for checking collisions over a sprite's whole area.

Unlike `TileMap`, `MapLayer` tile ids start at 1, with 0 meaning empty, and `add_layer` doesn't create a `transforms` array. `mode7()` draws tiles without an entry untransformed; to flip or rotate tiles, fill `transforms` yourself (one byte per tile).
