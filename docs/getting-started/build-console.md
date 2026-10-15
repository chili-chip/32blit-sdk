# Run on the console

Games for the VGC Zero are built as `.blit` files. ChiliChip OS keeps any number of them in flash, lists them on the home screen with their cover art, and keeps their saves when you update them.

You need the [console tools](install.md#console-tools) installed, and a console running ChiliChip OS. A new or wiped console can be set up with the [factory image](../console/flashing.md#setting-up-a-console).

## Build a `.blit`

With the template's preset:

```bash
cmake --preset vgc-blit
cmake --build out/build/vgc-blit
```

This produces `out/build/vgc-blit/game.blit`. The preset builds in Debug mode; for a release build add `-DCMAKE_BUILD_TYPE=Release` to the first command.

Without presets, from your game's folder:

```bash
mkdir build.vgc-blit
cd build.vgc-blit
cmake .. \
  -D32BLIT_DIR=../../32blit-sdk \
  -DPICO_SDK_PATH=../../pico-sdk \
  -DPICO_BOARD=chilichip_vgc \
  -DPICO_PLATFORM=rp2350-arm-s \
  -DCMAKE_TOOLCHAIN_FILE=../../32blit-sdk/pico2.toolchain \
  -DBLIT_EXECUTABLE_PICO_BLIT=1
make
```

| Option | Why |
| ------ | --- |
| `PICO_BOARD=chilichip_vgc` | the console's pins, display, audio and flash layout |
| `PICO_PLATFORM=rp2350-arm-s` | the RP2350's Arm cores. Anything else builds for the wrong chip and the console refuses the file. |
| `CMAKE_TOOLCHAIN_FILE=.../pico2.toolchain` | the Arm cross compiler and Pico SDK setup |
| `BLIT_EXECUTABLE_PICO_BLIT=1` | build an installable `.blit` instead of a standalone `.uf2` |

## Install it

1. Turn the console on and connect it to your computer with a USB cable. A drive named **CHILICHIP** appears.
2. Copy `game.blit` to the top of the drive.
3. Eject the drive (or unplug the cable).

The console installs the game with a progress bar, deletes the copy from the drive, and shows the game on the home screen. Select it and press A to play.

* Copying a new build of the same game (same title and author in `metadata.yml`) replaces the old one and keeps its saves.
* Several `.blit` files can be copied at once; they are installed one after another.
* Nothing is installed while the drive is still mounted on your computer, because the computer caches the drive's contents. Until you eject, the home screen can still start a `.blit` straight from the drive.

You can also install over USB serial with the 32blit tool, without ejecting:

```bash
32blit install game.blit
```

### If the console refuses the file

| Message | Cause | Fix |
| ------- | ----- | --- |
| Not a ChiliChip game | the file isn't a `.blit`, or was built for another device | check `PICO_PLATFORM=rp2350-arm-s` and `PICO_BOARD=chilichip_vgc`, and start from a clean build folder |
| Needs a system update | the game uses a newer API than the console's loader | update the console's loader (see [Flashing](../console/flashing.md)) |
| File damaged | the file is truncated or its checksum doesn't match | copy it again, and eject before unplugging |
| Not enough space | the games area of flash is full | delete a game from the home screen |

Files that fail stay on the drive so you can see which ones they were.

## Play and quit

* **Short HOME press** pauses the game and opens the system menu: resume, volume, brightness, battery and quit to home.
* **Hold HOME** for a second to quit to the home screen.

Either way of quitting calls your game's `on_quit` function first, so it can save. See [Saving progress](../tutorials/saving.md).

## Standalone builds

The template's `vgc` preset (the same CMake command without `-DBLIT_EXECUTABLE_PICO_BLIT=1`) builds a standalone `game.uf2` that contains the whole firmware and your game. Copy it to the console in BOOTSEL mode:

1. Switch the console off.
2. Hold the top **X** button and switch it on. A drive named `RP2350` appears.
3. Copy `game.uf2` onto it. The console restarts into your game.

Standalone builds are handy for quick hardware tests and for [debugging over SWD](../guides/debugging.md#debugging-on-the-console), but they **replace ChiliChip OS**: a standalone build is written from the start of flash, over the loader, the home screen and any games installed in the space it takes. Flash the [factory image](../console/flashing.md#setting-up-a-console) afterwards to get the home screen back, and reinstall any game that no longer starts. Saves and settings live on the USB drive at the end of flash, so they are kept.

## Building in CI

The template's GitHub Actions workflow builds every target on every push. The VGC jobs upload `game-blit` (ready to copy onto the console) and `game-uf2` as workflow artifacts and check that the `.blit` is built for the VGC Zero. Publishing a GitHub release attaches `game.blit` to it. See [Publishing a game](../guides/publishing.md).
