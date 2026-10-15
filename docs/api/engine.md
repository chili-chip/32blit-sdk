# Engine and game loop

`engine/engine.hpp`, `engine/version.hpp`, `engine/fast_code.hpp`

## Game functions

Your game defines these functions. The SDK calls them; you never call them yourself.

```cpp
void init();
void update(uint32_t time);
void render(uint32_t time);
```

`init()`
:   Called once at start-up, before the first update. Set the screen mode, load assets, and set up state.

`update(uint32_t time)`
:   Called every 10 ms (100 times a second). `time` is the game time in milliseconds of this step. If rendering falls behind, `update()` is called several times in a row to catch up, each with the time that step would have had, so game logic runs at the same speed whatever the frame rate. `buttons.pressed` and `buttons.released` are worked out just before each call.

`render(uint32_t time)`
:   Called once per frame, after any pending updates. Draw to [`screen`](surface.md). `time` is the current time in milliseconds.

All three are required: a game that doesn't define one fails to link.

### `on_quit`

```cpp
extern void (*on_quit)();
```

A function pointer, `nullptr` by default. Set it (usually in `init()`) to a function the SDK calls when the player quits:

* on the VGC Zero: **Quit to home** in the system menu, or holding HOME;
* on the desktop: closing the window.

It's the place to save progress. On the console, holding HOME waits for the current frame to finish before calling it; if the game is stuck for two more seconds, the loader quits without calling it. Switching the console off doesn't call it.

**API 0.5+.** See [Saves](saves.md#saving-when-the-player-quits).

## Screen mode

```cpp
enum ScreenMode { lores, hires, hires_palette };

void set_screen_mode(ScreenMode new_mode, Size bounds = {0, 0});
bool set_screen_mode(ScreenMode new_mode, PixelFormat format, Size bounds = {0, 0});
void set_screen_palette(const Pen *colours, int num_cols);
```

`set_screen_mode(mode)`
:   Switches the screen to a mode and replaces [`screen`](surface.md#screen) with a surface of the new size. Call it in `init()`; the game starts in `lores`.

| Mode | VGC Zero | Desktop |
| ---- | -------- | ------- |
| `lores` | 64×64 RGB565, shown at 2× | 64×64 RGB, shown at 2× |
| `hires` | 128×128 RGB565 | 128×128 RGB |
| `hires_palette` | not supported | 128×128 paletted |

`bounds`
:   Asks for a smaller screen than the mode's full size, for example `Size(128, 96)`. On the console, any size up to the mode's maximum is accepted. Leave it as `{0, 0}` for the full size.

`set_screen_mode(mode, format, bounds)`
:   The same with a specific [`PixelFormat`](surface.md#pixelformat). Returns `false`, leaving the screen unchanged, if the display can't do it. The VGC Zero accepts only `PixelFormat::RGB565`.

`set_screen_palette(colours, num_cols)`
:   Sets the palette for `hires_palette`. Has no effect on the VGC Zero.

## Time

```cpp
uint32_t now();
uint32_t now_us();
uint32_t us_diff(uint32_t from, uint32_t to);
```

`now()`
:   Milliseconds since the game started.

`now_us()`
:   A microsecond counter for measuring short intervals. Its starting point is undefined and it wraps around, so only use differences between two readings.

`us_diff(from, to)`
:   The microseconds from one `now_us()` reading to a later one, handling the counter wrapping around in between.

```cpp
uint32_t start = now_us();
expensive_thing();
debugf("took %lu us\n", (unsigned long)us_diff(start, now_us()));
```

## Random numbers

```cpp
uint32_t random();
```

Returns a random 32-bit number. On the console it comes from the RP2350's random number generator, so it's different on every run; there is no seed to set.

Call it as `blit::random()`. With `using namespace blit`, a bare `random()` is ambiguous with the C library's `random()` on Linux and macOS.

## Debug output

```cpp
void debug(std::string message);
int debugf(const char *format, ...);
```

`debug()` prints a string; `debugf()` formats like `printf()` and returns the number of characters. Output goes to the terminal on the desktop, and to USB serial and UART0 on the console. See [Debugging](../guides/debugging.md#printing-messages).

`debugf()` allocates a buffer for each call, so avoid it in tight loops.

## Launch path

```cpp
const char *get_launch_path();
```

The file the game was opened with, if a launcher started it to open a file (see `filetypes` in [metadata](../guides/metadata.md#fields)), or `nullptr`. On the desktop, set it with `--launch_path <file>`.

## Metadata and version

```cpp
const char *get_version_string();
const char *get_build_date();

struct GameMetadata {
  const char *title;
  const char *author;
  const char *description;
  const char *version;
  const char *url;
  const char *category;
};

GameMetadata get_metadata();
```

`get_version_string()`
:   The SDK version the game was built with, from `git describe`, for example `v1.0.0-3-ga349786`.

`get_build_date()`
:   When the SDK was built.

`get_metadata()`
:   The game's own fields from [`metadata.yml`](../guides/metadata.md).

## Fast code

```cpp
#define blit_fast_code(fn)
#define blit_no_inline_fast_code(fn)
```

Wrap a function's name in its definition to run it from RAM on the console instead of flash, which avoids cache misses in hot code:

```cpp
void blit_fast_code(draw_row)(int y) {
  ...
}
```

`blit_no_inline_fast_code` also stops the compiler inlining it into a caller in flash. Both do nothing on the desktop. RAM is limited, so use them only where measurements show it helps. See [Performance](../guides/performance.md#cpu-tips).

## `tick()`

```cpp
int tick(uint32_t time);
```

The SDK's main loop calls this to run timers, tweens and pending updates. Games don't call it.
