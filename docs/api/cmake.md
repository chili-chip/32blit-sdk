# CMake functions

The SDK's build is CMake. A game's `CMakeLists.txt` finds the SDK, declares the game with `blit_executable()`, and adds its assets and metadata. The same file builds for the desktop and the console; which one depends on how CMake is configured (see [Desktop builds](../getting-started/build-desktop.md) and [Run on the console](../getting-started/build-console.md)).

## A game's `CMakeLists.txt`

This is the template's, slightly shortened:

```cmake title="CMakeLists.txt"
cmake_minimum_required(VERSION 3.15...3.31)
project(game LANGUAGES C CXX ASM)

set(PROJECT_SOURCE game.cpp game.hpp)

if(MSVC)
  add_compile_options("/W4" "/wd4244" "/wd4324" "/wd4458" "/wd4100")
else()
  add_compile_options("-Wall" "-Wextra" "-Wdouble-promotion" "-Wno-unused-parameter")
endif()

find_package(32BLIT CONFIG REQUIRED PATHS ../32blit-sdk $ENV{PATH_32BLIT_SDK})

blit_executable(${PROJECT_NAME} ${PROJECT_SOURCE})
blit_assets_yaml(${PROJECT_NAME} assets.yml)
blit_metadata(${PROJECT_NAME} metadata.yml)
```

The project name (`game`) becomes the output's name: `game.blit`, `game.uf2`, or the desktop executable.

## Finding the SDK

```cmake
find_package(32BLIT CONFIG REQUIRED PATHS ../32blit-sdk $ENV{PATH_32BLIT_SDK})
```

This loads `32blit-config.cmake` from the SDK, which checks the [32blit tools](../getting-started/install.md) are installed (version 0.7.3 or newer), builds the engine, and defines the functions below. It's found, in order, from:

* `-D32BLIT_DIR=/path/to/32blit-sdk` on the CMake command line (the template's presets set this);
* the paths listed: a `32blit-sdk` folder next to your game's folder, then the `PATH_32BLIT_SDK` environment variable.

It also sets C++17 and, if you didn't choose one, a `Release` build type.

## Which platform is built

The SDK picks the platform from how CMake was configured:

| Configured with | Builds |
| --------------- | ------ |
| `PICO_SDK_PATH` and a Pico toolchain file | the VGC Zero (or another RP2040/RP2350 board, from `PICO_BOARD`) |
| `CMAKE_TOOLCHAIN_FILE=.../32blit.toolchain` | the original 32blit console |
| the Emscripten toolchain | a web page (`.html`, `.js` and `.wasm`) |
| anything else | a desktop executable using SDL2 |

For the VGC Zero, the settings are:

| Variable | Value |
| -------- | ----- |
| `PICO_SDK_PATH` | path to the Pico SDK (ChiliChip's fork, see [Install](../getting-started/install.md#console-tools)) |
| `PICO_BOARD` | `chilichip_vgc` |
| `PICO_PLATFORM` | `rp2350-arm-s` |
| `CMAKE_TOOLCHAIN_FILE` | `<32blit-sdk>/pico2.toolchain` |
| `BLIT_EXECUTABLE_PICO_BLIT` | `1` for an installable `.blit`; leave unset for a standalone `.uf2` |

## `blit_executable`

```cmake
blit_executable(<name> <sources...> [PICO_BLIT | PICO_STANDALONE_UF2] [PICO_BLIT_OFFSET_KB <kb>])
```

Declares the game. Use it instead of `add_executable()`; it links the engine and the platform's code and sets up the output files.

`<sources>`
:   Your `.cpp` files (and optionally headers).

`PICO_BLIT`
:   On Pico builds, build a `.blit` that ChiliChip OS installs and runs. Overrides `BLIT_EXECUTABLE_PICO_STANDALONE_UF2`.

`PICO_STANDALONE_UF2`
:   On Pico builds, build a `.uf2` containing the whole firmware and the game, which [replaces ChiliChip OS](../getting-started/build-console.md#standalone-builds). This is the default when neither is given.

`PICO_BLIT_OFFSET_KB <kb>`
:   Where in flash the `.blit` is linked to run from, in KB: at least 256 and a multiple of 64. The default for the RP2350 is 4096, which the loader maps onto wherever it installs the game, so leave it alone.

Instead of the arguments, you can set these variables before `blit_executable()`, or with `-D` on the command line, to change every target at once:

| Variable | Same as |
| -------- | ------- |
| `BLIT_EXECUTABLE_PICO_BLIT` | `PICO_BLIT` |
| `BLIT_EXECUTABLE_PICO_STANDALONE_UF2` | `PICO_STANDALONE_UF2` |
| `BLIT_EXECUTABLE_PICO_BLIT_OFFSET_KB` | `PICO_BLIT_OFFSET_KB` |
| `BLIT_EXECUTABLE_INTERNAL_FLASH` | `INTERNAL_FLASH` (original 32blit only) |

Arguments win over the variables.

### Outputs

| Platform | Files in the build folder |
| -------- | ------------------------- |
| VGC Zero, `.blit` | `<name>.blit`, plus `<name>.elf`, `.bin` and `.dis` (a disassembly) |
| VGC Zero, standalone | `<name>.uf2`, plus `<name>.elf`, `.bin`, `.hex`, `.dis` and `.map` |
| Desktop | `<name>` (`<name>.exe` on Windows, `<name>.app` on macOS) |
| Web | `<name>.html`, `<name>.js`, `<name>.wasm` |

The template sets the `.elf` suffix on desktop executables too, so on Linux the game is `game.elf`.

## `blit_assets_yaml`

```cmake
blit_assets_yaml(<target> <file>)
```

Packs the assets listed in `<file>` (usually `assets.yml`) into C++ source, compiles it into the target, and adds the output folder to the include path so you can `#include "assets.hpp"`. See [Assets](../guides/assets.md) for the file's format.

* The list is read when CMake configures, and changes to `assets.yml` make CMake reconfigure automatically.
* Assets are re-packed when any listed file changes.
* Paths in the file are relative to the folder containing the `CMakeLists.txt`; keep `assets.yml` next to it.

Call it once per target. To split assets into several files, list several outputs in one `assets.yml`.

## `blit_metadata`

```cmake
blit_metadata(<target> <file>)
```

Adds the game's title, author, version, icon, splash and other fields from `<file>` (usually `metadata.yml`). See [Metadata](../guides/metadata.md).

* For a `.blit`, the metadata is appended to the file after it's built. An 8×8 icon and a 128×96 splash are required.
* For a standalone `.uf2`, it's added as Pico binary info, readable with `picotool info`.
* On the desktop it provides [`get_metadata()`](engine.md#metadata-and-version), and on macOS it sets the app bundle's name, version and icon.

Call it **after** `blit_executable()` for the same target: the `.blit` build checks a property that `blit_executable()` sets.

## Several games in one project

One `CMakeLists.txt` can build several games, each with its own `blit_executable()`. Give them separate folders with `add_subdirectory()` when they have their own `assets.yml` or `metadata.yml`, because the generated `assets.hpp` and `metadata.cmake` go into the current binary folder. This is how [`docs/examples/CMakeLists.txt`](https://github.com/chili-chip/32blit-sdk/blob/master/docs/examples/CMakeLists.txt) builds all the examples.

## Presets

The template has a `CMakePresets.json` with three presets:

| Preset | Builds |
| ------ | ------ |
| `linux` | a desktop build with the system's GCC |
| `vgc` | a standalone `.uf2` for the VGC Zero |
| `vgc-blit` | an installable `.blit` for the VGC Zero |

```bash
cmake --preset vgc-blit
cmake --build out/build/vgc-blit
```

They expect `32blit-sdk` and `pico-sdk` next to the game's folder, and Debug builds. Editors such as VS Code (with the CMake Tools extension) and Visual Studio list the presets for you. To add your own, for example a release build, copy one and change `CMAKE_BUILD_TYPE`.
