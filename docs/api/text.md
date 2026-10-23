# Text and fonts

`graphics/surface.hpp`, `graphics/font.hpp`

See the [text tutorial](../tutorials/text.md) for examples.

## Drawing text

```cpp
void text(std::string_view message, const Font &font, const Point &p,
          bool variable = true, TextAlign align = TextAlign::top_left);
void text(std::string_view message, const Font &font, const Rect &r,
          bool variable = true, TextAlign align = TextAlign::top_left);
```

Draws `message` in the surface's `pen` colour.

`p`
:   The point to align the text to. With the default `top_left`, it's the top-left of the first character.

`r`
:   A rectangle to align the text inside. Text isn't clipped to it; use `clip` if you need that.

`variable`
:   `true` (the default) uses each character's own width; `false` gives every character the font's fixed width, so digits and columns line up.

`align`
:   Where the text sits relative to `p` or inside `r`. Multi-line text is aligned line by line.

`\n` in the message starts a new line, `font.char_h + font.spacing_y` pixels below.

## Measuring and wrapping

```cpp
Size measure_text(std::string_view message, const Font &font, bool variable = true);
std::string wrap_text(std::string_view message, int32_t width, const Font &font,
                      bool variable = true, bool words = true);
```

`measure_text()`
:   The width and height the text would take, including every line.

`wrap_text()`
:   Returns the message with line breaks added so no line is wider than `width` pixels. With `words = true` it breaks between words where it can; with `false` it breaks anywhere. Existing `\n`s are kept.

## `TextAlign`

```cpp
enum TextAlign {
  left = 0b0000, center_h = 0b0100, right = 0b1000,
  top = 0b0000, center_v = 0b0001, bottom = 0b0010,

  top_left, center_left, bottom_left,
  top_center, center_center, bottom_center,
  top_right, center_right, bottom_right,
};
```

Combine one horizontal and one vertical value with `|`, or use the named combinations.

## `Font`

```cpp
struct Font {
  constexpr Font(const uint8_t *data);   // from a packed font asset
  constexpr Font(const uint8_t *data, const uint8_t *char_w_variable,
                 uint8_t char_w, uint8_t char_h, uint8_t spacing_y = 1);

  const uint8_t *data;              // 1 bit per pixel glyph data
  uint8_t char_w, char_h;           // fixed character size
  uint8_t spacing_y;                // extra space between lines
  const uint8_t *char_w_variable;   // each character's own width
};
```

Fonts are bitmaps, one bit per pixel, drawn in the pen colour. Make your own from an image or a TrueType file with the [asset pipeline](../guides/assets.md#additional-options) (`type: font/image` or `font/font`), then:

```cpp
const Font my_font(asset_my_font);
```

## Built-in fonts

```cpp
extern const Font minimal_font;
extern const Font fat_font;
extern const Font outline_font;
```

All three are 6×8 pixels per character (fixed width) and cover printable ASCII. `minimal_font` and `outline_font` draw lower-case letters as small capitals.

| Font | Style |
| ---- | ----- |
| `minimal_font` | thin, compact |
| `fat_font` | bold |
| `outline_font` | outlined |
