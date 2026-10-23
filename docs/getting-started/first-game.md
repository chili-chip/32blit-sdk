# Your first game

New games start from the [ChiliChip game template](https://github.com/chili-chip/game-template). It's a working game with a build set up for every target, CI that builds it on every push, and a save example.

## Create the project

On GitHub, open the template and click **Use this template → Create a new repository**. Then clone your new repository into the same folder as `32blit-sdk`:

```bash
cd projects
git clone https://github.com/<you>/my-game
```

Without GitHub, you can download the template as a zip and unpack it there instead.

## What's in the template

```
my-game/
├── CMakeLists.txt      # project name, source files, assets and metadata
├── CMakePresets.json   # presets: linux, vgc-blit, vgc
├── assets.yml          # which images/maps/fonts to pack into the game
├── assets/             # the image files themselves
├── metadata.yml        # title, author, version, cover art for the home screen
├── game.cpp            # your game
├── game.hpp            # includes 32blit.hpp
├── .github/workflows/  # CI: builds every target, attaches game.blit to releases
└── .vscode/            # tasks and a debugger setup for VS Code
```

### `CMakeLists.txt`

The parts you edit are at the top:

```cmake
project(game LANGUAGES C CXX ASM)              # (1)!

set(PROJECT_SOURCE game.cpp game.hpp)          # (2)!

find_package (32BLIT CONFIG REQUIRED PATHS ../32blit-sdk $ENV{PATH_32BLIT_SDK})

blit_executable (${PROJECT_NAME} ${PROJECT_SOURCE})
blit_assets_yaml (${PROJECT_NAME} assets.yml)  # (3)!
blit_metadata (${PROJECT_NAME} metadata.yml)   # (4)!
```

1.  The project name is also the name of the output: `game.blit`, `game.uf2` and so on. Change `game` to your game's name.
2.  Add each new `.cpp` file here.
3.  Packs the files listed in `assets.yml` into `assets.cpp` and `assets.hpp`. See [Asset pipeline](../guides/assets.md).
4.  Adds the title, author, version and cover art from `metadata.yml`. See [Metadata and the home screen](../guides/metadata.md).

`find_package` looks for the SDK in `../32blit-sdk`, or in the folder named by the `PATH_32BLIT_SDK` environment variable. You can also pass `-D32BLIT_DIR=/path/to/32blit-sdk` to CMake.

### `metadata.yml`

```yaml
title: game title
description: game description
author: you
splash:
  file: assets/no-image.png
icon:
  file: assets/no-icon.png
version: v0.0.1
category: game
url: https://github.com/you/my-game
```

Change the title and author now. The title also names the folder your saves go in, so pick one you'll keep. The [metadata guide](../guides/metadata.md) covers the cover image and the other fields.

### `game.cpp`

The template's game counts how many times you press A and saves the count when you quit:

```cpp
#include "game.hpp"

using namespace blit;

struct SaveData {
    uint32_t presses = 0;
};

static SaveData save_data;

static void save_progress() {
    write_save(save_data);
}

void init() {
    set_screen_mode(ScreenMode::hires);

    if(!read_save(save_data))
        save_data = SaveData();

    on_quit = save_progress;
}

void render(uint32_t time) {
    screen.pen = Pen(0, 0, 0);
    screen.clear();

    screen.pen = Pen(255, 255, 255);
    screen.rectangle(Rect(0, 0, 320, 14));
    screen.pen = Pen(0, 0, 0);
    screen.text("Hello 32blit!", minimal_font, Point(5, 4));

    screen.pen = Pen(255, 255, 255);
    screen.text("A pressed " + std::to_string(save_data.presses) + " times", minimal_font, Point(5, 24));
}

void update(uint32_t time) {
    if(buttons.pressed & Button::A)
        save_data.presses++;
}
```

* `init()` switches to the full 128×128 screen, loads the save, and asks for `save_progress` to be called when the player quits.
* `render()` clears the screen and draws a title bar and the count. Drawing happens on `screen` with the current `pen` colour.
* `update()` checks whether A was pressed since the last update.

The [game loop tutorial](../tutorials/game-loop.md) goes through this properly.

## Build it

Build and run it on your computer first:

```bash
cd my-game
cmake --preset linux
cmake --build out/build/linux
./out/build/linux/game.elf
```

A 256×256 window opens with your game drawn at 2× scale. **Z** is the A button, so press it a few times, close the window, and run it again: the count is still there.

[Run on your computer](build-desktop.md) covers the desktop build in detail, and [Run on the console](build-console.md) puts the game on a VGC Zero.
