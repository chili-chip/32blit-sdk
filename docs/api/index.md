# API reference

Everything in the SDK is in the `blit` namespace and comes from one header:

```cpp
#include "32blit.hpp"

using namespace blit;
```

The reference is grouped by topic. Each page lists the functions and types with what they do, their parameters, and notes on how they behave on the VGC Zero.

## By topic

| Page | Covers |
| ---- | ------ |
| [Engine and game loop](engine.md) | `init`, `update`, `render`, `on_quit`; screen modes; time; random numbers; debug output; metadata |
| [Surface and drawing](surface.md) | `screen`, `Surface`, `Pen`, pixel formats, shapes, blitting, loading images, clipping and alpha |
| [Sprites](sprites.md) | spritesheets, `sprite()`, `SpriteTransform`, scaling |
| [Text and fonts](text.md) | `text()`, `measure_text()`, `wrap_text()`, `Font`, `TextAlign` |
| [Input](input.md) | `buttons`, `Button`, `pressed()`, `joystick`, `tilt`, `LED`, `vibration` |
| [Audio](audio.md) | `channels`, `AudioChannel`, waveforms, envelopes, `WavStream`, `MP3Stream` |
| [Saves](saves.md) | `read_save()`, `write_save()`, `remove_save()`, `on_quit` |
| [Files](files.md) | `File`, `list_files()`, directories, in-memory files |
| [System](system.md) | battery, USB, system settings and info (ChiliChip additions) |
| [Timers and tweens](timers-tweens.md) | `Timer`, `Tween` and the easing functions |
| [Particles](particles.md) | `Particle`, `ParticleGenerator` |
| [Maths and types](types.md) | `Point`, `Size`, `Rect`, `Vec2`, `Vec3`, `Mat3`, `Mat4`, `lerp()`, `pi` |
| [Tile maps and Mode 7](tilemap.md) | `TileMap`, `mode7()`, `Map` |
| [Menu](menu.md) | the `Menu` class |
| [Profiler](profiler.md) | `Profiler`, `ProfilerProbe`, `RunningAverage` |
| [Multiplayer](multiplayer.md) | linking two games |
| [CMake functions](cmake.md) | `blit_executable()`, `blit_assets_yaml()`, `blit_metadata()` and build options |

## Headers

`32blit.hpp` includes all of these. You can include one directly if you only need part of the SDK.

| Header | Contents |
| ------ | -------- |
| `engine/engine.hpp` | game loop, screen modes, time, random, debug |
| `engine/api.hpp` | `buttons`, `joystick`, `tilt`, `LED`, `vibration` |
| `engine/input.hpp` | `Button`, `ButtonState`, `pressed()` |
| `engine/save.hpp` | saves |
| `engine/file.hpp` | files |
| `engine/system.hpp` | battery, USB, system settings and info |
| `engine/timer.hpp`, `engine/tweening.hpp` | timers and tweens |
| `engine/particle.hpp` | particles |
| `engine/menu.hpp` | `Menu` |
| `engine/profiler.hpp`, `engine/running_average.hpp` | profiling |
| `engine/multiplayer.hpp` | multiplayer |
| `engine/version.hpp` | SDK version and game metadata |
| `engine/fast_code.hpp` | `blit_fast_code()` |
| `graphics/surface.hpp` | `Surface`, `Pen`, `PixelFormat`, `SpriteTransform`, `TextAlign` |
| `graphics/color.hpp` | `hsv_to_rgba()` |
| `graphics/font.hpp` | `Font` and the built-in fonts |
| `graphics/tilemap.hpp`, `graphics/mode7.hpp` | tile maps |
| `graphics/jpeg.hpp` | JPEG decoding |
| `audio/audio.hpp`, `audio/wav-stream.hpp`, `audio/mp3-stream.hpp` | audio |
| `types/*.hpp` | `Point`, `Size`, `Rect`, `Vec2`, `Vec3`, `Mat3`, `Mat4`, `Map` |
| `math/*.hpp` | `pi`, `lerp()` |

## Conventions

* **Time** is in milliseconds as `uint32_t`, unless a name ends in `_us` (microseconds).
* **Positions and sizes** on surfaces are whole pixels (`Point`, `Size`, `Rect`). Physics and smooth movement use floats (`Vec2`).
* **Colours** are `Pen`s with 8-bit red, green, blue and alpha.
* **Global state** such as `screen`, `buttons` and `channels` is a variable you read or write directly.

On the VGC Zero, the engine runs inside the loader and your game calls into it through a table of functions, so the same `.blit` works with newer loaders. The few calls added by ChiliChip (marked **API 0.3+** and so on) need a loader at least that new; see [Publishing a game](../guides/publishing.md#compatibility-with-players-consoles).
