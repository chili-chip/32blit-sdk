# 8. Tile maps

![Tile map](../assets/screenshots/tilemap.png){ .screenshot align=right }

Levels in platformers, top-down adventures and puzzle games are usually grids of tiles. A `TileMap` stores a grid of sprite numbers and draws the visible part of it, scrolled, scaled or even rotated.

```cpp title="tilemap.cpp"
--8<-- "tilemap/tilemap.cpp"
```

## Rules for tile maps

* The map's width and height, in tiles, must be **powers of two**: 8, 16, 32, 64, 128 and so on. The level in the example is 32×16.
* Each tile is one 8×8 sprite, and the tile's value is the sprite's number in the sheet.
* The spritesheet must be **128 pixels wide** (16 sprites per row), because the map works out a tile's position in the sheet assuming 16 per row.
* Tile values are bytes, so a map can use up to 256 different tiles.

## Building a map

A map needs an array of tile numbers, `width × height` long, arranged row by row:

```cpp
uint8_t tiles[map_w * map_h];
...
map = new TileMap(tiles, nullptr, Size(map_w, map_h), sheet);
```

The second argument is an optional array of per-tile transforms (flips and rotations), the same size as the tiles; `nullptr` means none. The map doesn't copy your arrays, so they must stay alive while it's used. Because it reads them directly, you can change a tile while the game runs just by writing to the array, as the example does when you take a coin.

Writing the level as text, with one character per tile, makes it much easier to see and edit than a list of numbers.

## Drawing and scrolling

```cpp
map->transform = Mat3::translation(camera);
map->draw(&screen, Rect(0, 0, 128, 128));
```

`draw()` fills the given rectangle of the screen with the map. For each pixel it works out which point of the map to show using `transform`, a [`Mat3`](../api/types.md#mat3) that turns screen positions into map positions (in pixels). Translating by the camera position scrolls; you can also scale and rotate:

```cpp
// zoom in 2x around the camera, and rotate
map->transform = Mat3::translation(camera)
               * Mat3::translation(Vec2(64, 64))
               * Mat3::rotation(angle)
               * Mat3::scale(Vec2(0.5f, 0.5f))
               * Mat3::translation(Vec2(-64, -64));
```

Drawing works one scanline at a time, so `draw()` also accepts a function that returns a different transform for each line of the screen. That's how the classic "Mode 7" floor effects are made; see [Tile maps and Mode 7](../api/tilemap.md).

### Empty tiles and what's outside

* Tiles equal to `empty_tile_id` aren't drawn, so whatever you drew first (a sky, a parallax background) shows through.
* `repeat_mode` says what to show beyond the map's edges: `NONE` (nothing, the default), `REPEAT` (wrap around), `CLAMP_TO_EDGE` (repeat the edge tiles) or `DEFAULT_FILL` (fill with `default_tile_id`).

## Reading the map

To find what's at a position, divide pixels by 8 to get tiles:

```cpp
Point tile = Point(player_pos) / 8;
uint8_t t = map->tile_at(tile);
if(t == WATER)
  splash();
```

`tile_at()` follows `repeat_mode` for positions outside the map, and returns 0 there when the mode is `NONE`. `offset()` gives the index into your `tiles` array, or `-1` outside the map.

This is the basis of collision: before moving the player, check the tiles at the corners of where they would be, and don't move if any is solid.

## Maps made in Tiled

For bigger levels, draw them in [Tiled](https://www.mapeditor.org/) and let the asset pipeline pack them:

```yaml title="assets.yml"
assets.cpp:
  assets/level.tmx:
    name: asset_level
    type: map/tiled
    output_struct: true
```

```cpp
map = TileMap::load_tmx(asset_level, sheet);
```

`load_tmx()` returns `nullptr` if the map isn't 8-bit or its size isn't a power of two. It copies the tiles into RAM by default so you can change them; pass `0` as the `flags` argument to use them straight from flash when the level never changes. Maps with several layers load one layer per `TileMap`, picked with the `layer` argument.

## Try it

* Make the camera follow a player sprite that walks on the grass and can't walk through bricks.
* Add a second, slower-scrolling map behind the first for parallax.
* Add a `transforms` array and flip some flowers to make the level look less repetitive.

Next: [make a complete game](complete-game.md).
