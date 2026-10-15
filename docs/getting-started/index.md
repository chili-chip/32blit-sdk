# Getting started

This section takes you from nothing to your own game running on the VGC Zero. It takes four steps:

1. **[Install the tools](install.md):** a C++ compiler, CMake, the 32blit Python tools, and for the console the Arm compiler and the Pico SDK.
2. **[Create your first game](first-game.md)** from the ChiliChip game template, and learn what each file in it does.
3. **[Run it on your computer](build-desktop.md).** The desktop build opens a 128×128 window, the same size as the console's screen, so you can work fast and use a normal debugger.
4. **[Run it on the console](build-console.md).** Build a `.blit` file, copy it onto the console's USB drive, and play it from the home screen.

## What you need

* A computer running Linux, macOS or Windows. Linux, or Windows with WSL, is the smoothest route for console builds.
* Some C++. The SDK uses plain C++17: structs, functions and the standard library. You don't need to know templates or anything advanced.
* For console builds: a VGC Zero with ChiliChip OS and a USB-C data cable. A console that doesn't have the OS yet can be set up with the [factory image](../console/flashing.md#setting-up-a-console).

## How a game is put together

Every game is one CMake project that links against the SDK. You write three functions:

| Function | When it runs | What it's for |
| -------- | ------------ | ------------- |
| `init()` | once, at start-up | load assets, set the screen mode, set up state |
| `update(time)` | 100 times a second | read input, move things, game logic |
| `render(time)` | once per frame | draw everything to `screen` |

The same source builds for several targets, picked when you run CMake:

| Target | Output | Use it for |
| ------ | ------ | ---------- |
| Desktop (SDL2) | an executable | everyday development and debugging |
| VGC Zero `.blit` | `game.blit` | the normal way to ship: install it from the console's USB drive |
| VGC Zero standalone | `game.uf2` | quick hardware tests and debugging over SWD (replaces ChiliChip OS) |
| Web (Emscripten) | `.html` + `.wasm` | playable demos in a browser |

Once your game runs, the [tutorials](../tutorials/index.md) show how to use each part of the SDK.
