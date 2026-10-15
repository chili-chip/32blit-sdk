# Run on your computer

The desktop build uses SDL2 to open a window, read the keyboard or a gamepad, and play sound. It's the fastest way to work: rebuilds take seconds, and you can use your normal debugger.

The window shows the console's 128×128 screen at 2× scale. Resize it and the picture scales to fit.

## Build

With the template's preset:

```bash
cmake --preset linux
cmake --build out/build/linux
./out/build/linux/game.elf
```

Or with plain CMake, from your game's folder:

```bash
mkdir build
cd build
cmake .. -D32BLIT_DIR=../../32blit-sdk
make
./game.elf
```

On Windows with Visual Studio, open the game's folder and let Visual Studio's CMake support configure it; see [Visual Studio](../setup/visual-studio.md).

## Controls

| Console | Keyboard |
| ------- | -------- |
| D-pad | ++arrow-up++ ++arrow-down++ ++arrow-left++ ++arrow-right++ or ++w++ ++a++ ++s++ ++d++ |
| A | ++z++ or ++u++ |
| B | ++x++ or ++i++ |
| X | ++c++ or ++o++ |
| Y | ++v++ or ++p++ |
| MENU | ++2++ or ++esc++ |
| HOME | ++1++ |

A game controller works too: its d-pad and face buttons map to the same buttons, Start is MENU, Back is HOME, and the left stick sets `joystick`.

The desktop build has no system menu or home screen, so HOME is just a button your game can read. Closing the window counts as quitting: `on_quit` is called, so your game can save.

## Command line options

| Option | Effect |
| ------ | ------ |
| `--fullscreen` | start full screen |
| `--position x,y` | place the window |
| `--size w,h` | set the window size |
| `--launch_path <file>` | pretend the game was opened with a file, as `get_launch_path()` returns |
| `--listen` / `--connect <addr>` | link two copies for [multiplayer](../api/multiplayer.md) |
| `--info` | print the game's metadata and exit |
| `--credits` | print the 32blit contributors and exit |

The title bar shows `[SLOW]` when `update()` can't keep up with 100 updates a second, and `[FROZEN]` if it stops returning.

## Differences from the console

The desktop build runs the same engine code, but it is not an emulator:

* **Speed.** Your computer is many times faster than the RP2350. A game that runs smoothly on the desktop can still drop frames on the console, so test on hardware regularly. See [Performance and memory](../guides/performance.md).
* **Memory.** The desktop has no 448 KB limit. Allocations that work here can fail on the console.
* **Pixel format.** The desktop screen is 24-bit RGB; the console's is 16-bit RGB565, so subtle gradients band slightly more on the console.
* **Files and saves.** Files are read from the folder the game runs in. Saves go to SDL's per-user folder for the game's author and title, for example `~/.local/share/<author>/<title>/` on Linux, rather than the console's USB drive.
* **System calls.** The battery, USB, settings and system info calls report "not available". Your game should cope with that, which it needs to do on older firmware anyway.

## Build for the web

The same code builds to WebAssembly with Emscripten, which is handy for putting a playable demo online. See [Web (Emscripten)](../setup/emscripten.md).
