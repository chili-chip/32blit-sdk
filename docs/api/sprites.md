# Sprites

`graphics/surface.hpp`

Sprites are 8×8 cells of a spritesheet surface. Set `screen.sprites` (or any surface's `sprites`) to a loaded sheet, then draw with `sprite()`. See the [sprites tutorial](../tutorials/sprites.md).

```cpp
screen.sprites = Surface::load(asset_sprites);
...
screen.sprite(0, Point(10, 10));
```

## Choosing a sprite

Every `sprite()` call takes the sprite in one of three forms:

| Type | Meaning | Example |
| ---- | ------- | ------- |
| `uint16_t index` | the *n*th 8×8 cell, counting left to right along each row of the sheet | `5` |
| `Point cell` | the cell at a column and row | `Point(5, 0)` |
| `Rect cells` | a block of cells, in cells: position and size | `Rect(0, 1, 2, 2)` is 16×16 pixels |

Indices depend on the sheet's width: a 128-pixel-wide sheet has 16 cells per row, so index 16 is the first cell of the second row.

```cpp
Rect sprite_bounds(uint16_t index);
Rect sprite_bounds(const Point &cell);
Rect sprite_bounds(const Rect &cells);
```

Called on the sheet, these return the pixel rectangle of a sprite, for use with [`blit()`](surface.md#blitting).

## Drawing

```cpp
void sprite(uint16_t sprite, const Point &position, uint8_t transform = 0);
void sprite(uint16_t sprite, const Point &position, const Point &origin, uint8_t transform = 0);
void sprite(uint16_t sprite, const Point &position, const Point &origin, float scale, uint8_t transform = 0);
void sprite(uint16_t sprite, const Point &position, const Point &origin, const Vec2 &scale, uint8_t transform = 0);
```

Each also exists with `const Point &sprite` and `const Rect &sprite` in place of the index.

`position`
:   Where to draw. Without an `origin`, it's the sprite's top-left corner.

`origin`
:   A point in the sprite, in pixels from its top-left corner, that is placed at `position`. Scaling happens around it. Use the sprite's centre (`Point(4, 4)` for an 8×8) to position and scale by the centre.

`scale`
:   A size multiplier, the same on both axes (`float`) or separate (`Vec2`). Scaling uses nearest-neighbour sampling, so whole numbers look best for pixel art.

`transform`
:   A `SpriteTransform` (below), applied before scaling.

Sprites blend like any blit: transparent pixels in the sheet are skipped, and the surface's `alpha` and `mask` apply. If `sprites` is `nullptr`, nothing is drawn.

## `SpriteTransform`

```cpp
enum SpriteTransform {
  NONE       = 0b000,
  HORIZONTAL = 0b001, // mirror left-right
  VERTICAL   = 0b010, // mirror top-bottom
  XYSWAP     = 0b100, // swap x and y
  R90        = 0b101, // rotate 90° clockwise
  R180       = 0b011, // rotate 180°
  R270       = 0b110, // rotate 270° clockwise (90° anticlockwise)
};
```

Every rotation and mirror is a combination of the three basic operations, applied in the order horizontal flip, vertical flip, x/y swap. Combine values with `|`, for example `HORIZONTAL | VERTICAL` is the same as `R180`.

Use `XYSWAP`, `R90` and `R270` only on square sprites: the drawn area keeps the source's width and height, so a non-square block comes out cropped and distorted.

## Example

```cpp
// a 16x16 character made of 2x2 cells at the top-left of the sheet, facing left
screen.sprite(Rect(0, 0, 2, 2), Point(x, y), SpriteTransform::HORIZONTAL);

// a coin spinning by squashing it horizontally
float squash = fabsf(sinf(time / 200.0f));
screen.sprite(COIN, Point(64, 64), Point(4, 4), Vec2(squash, 1.0f));
```
