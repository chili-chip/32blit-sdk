# 4. Text

![Text](../assets/screenshots/text.png){ .screenshot align=right }

Scores, menus and dialogue all need text. The SDK has three built-in fonts and can measure, align and wrap text for you.

```cpp title="text.cpp"
--8<-- "text/text.cpp"
```

## Drawing text

```cpp
screen.pen = Pen(255, 255, 255);
screen.text("Score: " + std::to_string(score), minimal_font, Point(2, 2));
```

Text is drawn in the current pen colour, with its top-left corner at the point. Build strings with `std::string` and `std::to_string()`. A `\n` starts a new line.

## Fonts

| Font | Looks like |
| ---- | ---------- |
| `minimal_font` | thin capitals, the most compact |
| `fat_font` | bold, for titles and scores |
| `outline_font` | outlined capitals |

All three are 8 pixels tall; `minimal_font` and `outline_font` draw lower-case letters as capitals. You can make your own font from an image or a TrueType file with the [asset pipeline](../guides/assets.md#additional-options) and use it like the built-in ones:

```cpp
const Font my_font(asset_my_font);
screen.text("Hello", my_font, Point(0, 0));
```

### Variable and fixed width

By default each character takes only as much space as it needs. Pass `false` as the fourth argument to give every character the same width, so columns of numbers line up:

```cpp
screen.text("0042", minimal_font, Point(100, 2), false);
```

## Alignment

Pass a [`TextAlign`](../api/text.md#textalign) to place text relative to a point or inside a rectangle:

```cpp
// centred in the screen
screen.text("PAUSED", fat_font, Rect(0, 0, 128, 128), true, TextAlign::center_center);

// right-aligned to a point: the text ends at x = 126
screen.text("999", minimal_font, Point(126, 2), true, TextAlign::top_right);
```

The horizontal choices are `left`, `center_h` and `right`, the vertical ones `top`, `center_v` and `bottom`, and the combinations are named `top_left`, `center_center`, `bottom_right` and so on.

## Measuring

`measure_text()` returns the [`Size`](../api/types.md#size) text would take, without drawing it. Use it to draw a box behind a label, or to centre something next to it:

```cpp
Size size = screen.measure_text("measured", minimal_font);
screen.pen = Pen(214, 40, 40);
screen.rectangle(Rect(Point(80, 18), size));
```

## Wrapping

`wrap_text()` inserts line breaks so text fits a width, breaking between words. Draw the result into a rectangle:

```cpp
std::string wrapped = screen.wrap_text(story, 124, minimal_font);
screen.text(wrapped, minimal_font, Rect(2, 68, 124, 58));
```

Wrapping takes some work, so for text that doesn't change, wrap it once (in `init()` or when it changes) and keep the result.

## Try it

* Show a score in the top-right corner that stays right-aligned as it grows.
* Make a dialogue box: a rectangle along the bottom with wrapped text that appears one character at a time (draw `wrapped.substr(0, n)` and increase `n` in `update()`).

Next: [sound](sound.md).
