# 3. Sprites and assets

![Sprites](../assets/screenshots/sprites.png){ .screenshot align=right }

Most games draw with **sprites**: small images cut from a larger **spritesheet**. This lesson packs a sheet into the game, then draws, animates, flips, rotates and scales sprites from it.

## The spritesheet

The examples share this sheet, `docs/examples/shared/sprites.png`, shown here at 4× size:

![The examples' spritesheet](../examples/shared/sprites.png){ .screenshot style="width:512px" }

It's an ordinary PNG with transparency, 128×8 pixels, holding sixteen 8×8 sprites. Sprites are numbered from the top-left, along each row: the chili is sprite 0, the player is 1 and 2, the heart 3, and so on.

The SDK works in 8×8 cells. Bigger sprites are blocks of cells, like a 16×16 sprite made of 2×2 cells. Keep sheets 128 pixels wide (16 cells per row) so they also work with [tile maps](tilemaps.md).

## Packing assets into the game

Images aren't loaded from files at run time. The 32blit tool converts them into C++ arrays at build time, which end up in flash with your code. List them in `assets.yml`:

```yaml title="assets.yml"
assets.cpp:
  assets/sprites.png:
    name: asset_sprites
```

and add the file to your build in `CMakeLists.txt`:

```cmake
blit_assets_yaml(${PROJECT_NAME} assets.yml)
```

The build creates `assets.cpp` and `assets.hpp`. Include the header to use the data:

```cpp
#include "assets.hpp"   // declares asset_sprites and asset_sprites_length
```

The [asset pipeline guide](../guides/assets.md) covers palettes, transparency colours, maps, fonts and raw data.

## Drawing sprites

```cpp title="sprites.cpp"
--8<-- "sprites/sprites.cpp"
```

### Loading the sheet

```cpp
screen.sprites = Surface::load(asset_sprites);
```

`Surface::load` unpacks the image into a new surface in RAM. Setting `screen.sprites` makes it the sheet that `screen.sprite()` draws from. Load once, in `init()`: loading allocates memory each time.

If your game has several sheets, keep each `Surface *` and switch `screen.sprites` between them as you draw.

### Choosing a sprite

`screen.sprite(which, position)` draws a sprite with its top-left corner at `position`. There are three ways to say which sprite:

| `which` | Means |
| ------- | ----- |
| `5` | sprite number 5, counting along the rows |
| `Point(5, 0)` | the cell at column 5, row 0 |
| `Rect(1, 0, 2, 2)` | a block of cells, 2 wide and 2 tall, starting at column 1, row 0: a 16×16 sprite |

### Animation

Animation is choosing a different sprite as time passes:

```cpp
uint16_t frame = (time / 200) % 2 ? PLAYER_WALK_1 : PLAYER_WALK_2;
```

For longer animations, keep the frames in an array:

```cpp
const uint16_t walk[] = {16, 17, 18, 17};
uint16_t frame = walk[(time / 120) % 4];
```

### Flipping and rotating

The last argument is a [`SpriteTransform`](../api/sprites.md#spritetransform): `HORIZONTAL` and `VERTICAL` flips, and `R90`, `R180`, `R270` rotations. Flip a character horizontally to make them face the other way, so you only need to draw them facing one way.

### Scaling

With an origin and a scale, the sprite is scaled around the origin, measured in pixels from the sprite's top-left corner. Use the middle of the sprite (`Point(4, 4)` for an 8×8) to grow it in place; the position is then where the middle goes.

```cpp
screen.sprite(COIN, Point(20, 106), Point(4, 4), 2.0f);           // twice the size
screen.sprite(BOMB, Point(84, 106), Point(4, 4), Vec2(4.0f, 2.0f)); // stretched
```

Scaling looks best with whole-number factors on pixel art.

## Blitting whole images

`screen.blit()` copies any rectangle of one surface onto another, without the 8×8 grid. It's how you draw a background picture or a title image:

```cpp
Surface *title = Surface::load(asset_title);
...
screen.blit(title, Rect(0, 0, 128, 64), Point(0, 20));
```

`stretch_blit()` does the same while resizing to a destination rectangle. See [Surface](../api/surface.md#blitting).

## Memory

A loaded sheet is uncompressed: an 8-bit paletted image takes one byte per pixel, plus its palette. A 128×128 sheet is 16 KB. That matters on the console, where a game has 448 KB of RAM in total. If a sheet never changes, `Surface::load_read_only()` points straight at the data in flash instead of copying it (for unpacked images only; set `packed: no` on the asset).

## Try it

* Draw a row of hearts for lives, as a game's HUD would.
* Make the walking player in the example move up and down too, and use `R90`/`R270` so they face their direction.
* Draw your own 8×8 sprite in an image editor, add it to the sheet, and show it.

Next: [text](text.md).
