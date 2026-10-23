# Surface and drawing

`graphics/surface.hpp`, `graphics/color.hpp`, `graphics/blend.hpp`, `graphics/jpeg.hpp`

A `Surface` is a rectangle of pixels you can draw on. The screen is one; spritesheets and off-screen buffers are others. All drawing goes through a surface and uses its current `pen`, `alpha`, `clip` and `mask`.

## `screen`

```cpp
extern Surface screen;
```

The surface for the current frame. It's replaced by [`set_screen_mode()`](engine.md#screen-mode), so don't keep a pointer to its data across a mode change. Draw to it in `render()`.

## `Pen`

```cpp
struct Pen {
  uint8_t r, g, b, a;

  constexpr Pen();                                        // transparent black
  constexpr Pen(int r, int g, int b, int a = 255);        // 0-255
  constexpr Pen(float r, float g, float b, float a = 1.0f); // 0.0-1.0
  constexpr Pen(int a);                                   // a palette index, for paletted surfaces
};
```

A colour with alpha. `a` is opacity: 255 is solid, 0 is invisible.

Be careful with the single-argument form: `Pen(5)` is palette entry 5 (stored in `a`), not a grey.

```cpp
Pen hsv_to_rgba(float h, float s, float v);
```

Builds a pen from hue, saturation and value, each 0.0–1.0. Hue 0 is red, 1/3 green, 2/3 blue. Handy for rainbow effects and generated palettes.

## `PixelFormat`

```cpp
enum class PixelFormat {
  RGB = 0,    // 8 bits each of red, green, blue
  RGBA = 1,   // 8 bits each of red, green, blue, alpha
  P = 2,      // 8-bit index into the surface's palette
  M = 3,      // 8-bit mask (single channel)
  RGB565 = 4, // 16-bit: 5 red, 6 green, 5 blue (the VGC Zero's screen)
  BGR555 = 5, // 16-bit: 5 blue, 5 green, 5 red
};

static const uint8_t pixel_format_stride[]; // bytes per pixel for each format
```

## Fields

```cpp
struct Surface {
  uint8_t *data;              // pixel data
  Size bounds;                // size in pixels
  Rect clip;                  // drawing is limited to this rectangle
  uint8_t alpha = 255;        // global alpha for all drawing
  Pen pen;                    // colour for drawing operations
  PixelFormat format;
  uint8_t pixel_stride;       // bytes per pixel
  uint16_t row_stride;        // bytes per row
  Surface *mask = nullptr;    // optional per-pixel alpha mask
  Pen *palette = nullptr;     // for PixelFormat::P
  Surface *sprites = nullptr; // spritesheet used by sprite()
  uint8_t transparent_index = 0;
  uint16_t rows, cols;        // size in 8x8 cells
  ...
};
```

`pen`
:   The colour used by every shape and text call. Set it before drawing.

`alpha`
:   Multiplies the alpha of everything drawn, from 0 (nothing is drawn) to 255 (normal). Use it to fade a group of drawing in or out; set it back to 255 afterwards.

`clip`
:   Drawing is limited to this rectangle. It starts as the whole surface. `clear()` fills only the clip rectangle.

`mask`
:   An optional surface in `PixelFormat::M`, the same size as this one. Each mask byte multiplies the alpha of what's drawn on the matching pixel, so you can draw through a stencil, such as a spotlight.

`sprites`
:   The spritesheet that [`sprite()`](sprites.md) draws from.

`palette`, `transparent_index`
:   For paletted surfaces, the colours and which index is transparent.

## Creating surfaces

```cpp
Surface(uint8_t *data, const PixelFormat &format, const Size &bounds);
```

Wraps a buffer you own as a surface. The buffer must be `bounds.w * bounds.h * pixel_format_stride[format]` bytes:

```cpp
// an off-screen 64x64 RGBA buffer
static uint8_t buffer[64 * 64 * 4];
Surface offscreen(buffer, PixelFormat::RGBA, Size(64, 64));

offscreen.pen = Pen(0, 0, 0, 0);
offscreen.clear();
offscreen.pen = Pen(255, 0, 0);
offscreen.circle(Point(32, 32), 20);

// later, in render()
screen.blit(&offscreen, Rect(0, 0, 64, 64), Point(32, 32));
```

To blit an off-screen surface onto the console's screen, make it `RGBA`, `RGB` or `P`: blitting reads source pixels as pens, which `RGB565` surfaces aren't.

## Loading images

```cpp
static Surface *load(const uint8_t *image);
static Surface *load(const uint8_t *image, uint8_t *data, size_t data_size);
static Surface *load(const std::string &filename);
static Surface *load(const std::string &filename, uint8_t *data, size_t data_size);
static Surface *load_read_only(const uint8_t *image);
bool save(const std::string &filename);
```

`load(image)`
:   Loads an image packed by the [asset pipeline](../guides/assets.md), returning a new surface or `nullptr` if the data isn't a valid image. Packed images become 8-bit paletted (`P`) surfaces; the pixel data is allocated on the heap.

`load(filename)`
:   Loads a packed image file or a `.bmp` from [storage](files.md).

`data`, `data_size`
:   Optionally, a buffer of your own to unpack the pixels into instead of allocating one.

`load_read_only(image)`
:   Makes a surface that points straight at the image data in flash, using no RAM for pixels. Only works for images packed without bit-packing (`packed: no` in `assets.yml`), and you can't draw onto the result, but you can change its palette.

`save(filename)`
:   Writes the surface to a `.bmp` or `.blim` file. Returns `false` for other extensions or if the file can't be written.

```cpp
struct JPEGImage { Size size; uint8_t *data; };
JPEGImage decode_jpeg_buffer(const uint8_t *ptr, uint32_t len);
JPEGImage decode_jpeg_file(const std::string &filename);
```

Decodes a JPEG into an RGB buffer you then own (`delete[] image.data`). Wrap it with `Surface(image.data, PixelFormat::RGB, image.size)` to draw it. JPEG decoding needs a lot of RAM on the console.

## Shapes

All shapes use `pen`, and respect `clip`, `alpha` and `mask`.

| Function | Draws |
| -------- | ----- |
| `void clear()` | fills the clip rectangle |
| `void pixel(const Point &p)` | one pixel |
| `void h_span(Point p, int16_t count)` | a horizontal line of `count` pixels starting at `p` |
| `void v_span(Point p, int16_t count)` | a vertical line of `count` pixels starting at `p` |
| `void rectangle(const Rect &r)` | a filled rectangle |
| `void circle(const Point &centre, int32_t radius)` | a filled circle |
| `void line(const Point &p1, const Point &p2)` | a one-pixel line between two points |
| `void triangle(Point p1, Point p2, Point p3)` | a filled triangle; triangles sharing an edge don't overlap |
| `void polygon(std::vector<Point> points)` | a filled polygon |

There are no outline versions of the filled shapes. Draw outlines with `line()` or spans, for example a rectangle outline:

```cpp
void outline(const Rect &r) {
  screen.h_span(r.tl(), r.w);
  screen.h_span(Point(r.x, r.y + r.h - 1), r.w);
  screen.v_span(r.tl(), r.h);
  screen.v_span(Point(r.x + r.w - 1, r.y), r.h);
}
```

## Reading pixels

```cpp
Pen get_pixel(Point p);
Pen get_pixel(uint32_t offset);
uint8_t *ptr(int32_t x, int32_t y);
uint32_t offset(int32_t x, int32_t y);
```

`get_pixel()` returns a pixel as a pen whatever the format. `ptr()` points at a pixel's bytes and `offset()` gives its index, for working on the data directly.

## Blitting

```cpp
void blit(Surface *src, Rect src_r, Point dst_p);
void blit(Surface *src, const Rect &src_r, const Point &dst_p, int transforms);
void stretch_blit(Surface *src, const Rect &src_r, const Rect &dst_r);
void stretch_blit(Surface *src, const Rect &src_r, const Rect &dst_r, int transforms);
void stretch_blit_vspan(Surface *src, Point uv, uint16_t sc, Point p, int16_t dc);
```

`blit(src, src_r, dst_p)`
:   Copies the `src_r` rectangle of `src` to this surface with its top-left at `dst_p`, blending by the source's alpha (or palette transparency), `alpha` and `mask`.

`transforms`
:   A [`SpriteTransform`](sprites.md#spritetransform) to flip or rotate while copying.

`stretch_blit(src, src_r, dst_r)`
:   Copies and scales `src_r` to fill `dst_r`, using nearest-neighbour sampling.

`stretch_blit_vspan(src, uv, sc, p, dc)`
:   Draws one vertical column: `sc` source pixels down from `uv`, stretched to `dc` pixels down from `p`. It's the building block for raycasters and other column-based renderers.

## Custom pixel operations

```cpp
void custom_modify(Rect r, std::function<void(uint8_t *p, int16_t c)> f);
void custom_blend(Surface *src, Rect r, Point p,
                  std::function<void(uint8_t *psrc, uint8_t *pdest, int16_t c)> f);
```

For effects the built-in drawing can't do. Both clip the rectangle, then call `f` once per row with raw pointers to the row's first pixel and the row's width in pixels. Your function does all the pixel work, in the surface's own format: `pen`, `alpha` and `mask` aren't used.

```cpp
// darken the bottom half of the screen (RGB565 on the console)
screen.custom_modify(Rect(0, 64, 128, 64), [](uint8_t *p, int16_t c) {
  auto px = (uint16_t *)p;
  for(int i = 0; i < c; i++)
    px[i] = (px[i] >> 1) & 0x7bef;  // halve each channel
});
```

## Other

`void generate_mipmaps(uint8_t depth)`
:   Builds `depth` half-size copies of the surface in `mipmaps`, used by some 3D-style tile map drawing.

`void watermark()`
:   Draws the 32blit logo in the bottom-right corner.

## Blend functions

```cpp
using PenBlendFunc  = void(*)(const Pen *pen, const Surface *dest, uint32_t off, uint32_t cnt);
using BlitBlendFunc = void(*)(const Surface *src, uint32_t soff, const Surface *dest, uint32_t doff, uint32_t cnt, int32_t src_step);
using PenGetFunc    = Pen(*)(const Surface *surf, uint32_t off);
```

Each surface has `pbf`, `bbf` and `pgf` set from its format when it's created. They do the actual pixel writes for shapes, blits and `get_pixel()`. You can replace them with your own to change how drawing combines with what's there, for example additive blending; the built-in ones (`RGBA_RGB565`, `RGBA_RGBA`, `P_P` and so on) are in `graphics/blend.hpp`.
