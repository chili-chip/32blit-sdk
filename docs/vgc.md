# Building 32Blit SDK for ChiliChip VGC Zero

This guide explains how to build 32Blit SDK projects for **ChiliChip VGC Zero**, a **RP2350-based** video game console.

---

## Prerequisites

Install dependencies (Ubuntu / WSL):

```bash
sudo apt install git gcc g++ gcc-arm-none-eabi cmake make \
python3 python3-pip python3-setuptools python3-venv \
libsdl2-dev libsdl2-image-dev libsdl2-net-dev unzip
```

Install the 32Blit Python tools from the SDK's `tools/requirements.txt`. It installs the `32blit` command (which pulls in `construct`, `bitstring` and the other Python dependencies) and pins `bitstring<5`, because bitstring 5 removed `ConstBitStream`, which the tools still use.

Recent distributions (Debian 12, Ubuntu 23.04 and later) refuse `pip install` outside a virtual environment ("externally-managed-environment", [PEP 668](https://peps.python.org/pep-0668/)), so create one first:

```bash
python3 -m venv ~/venvs/32blit
source ~/venvs/32blit/bin/activate
python3 -m pip install -r tools/requirements.txt
```

Run the commands from the root of your `32blit-sdk` checkout, and activate the venv (`source ~/venvs/32blit/bin/activate`) in every new terminal before building.

---

## Directory Structure

Recommended project layout:

```
project_root/
├── 32blit-sdk/
├── pico-sdk/
├── pico-extras/
└── your-project/
```

---

## Configuring the Build for ChiliChip VGC Zero

Create a build directory:

```bash
cd your-project
mkdir build.vgc
cd build.vgc
```

Run `cmake` with the board and platform specified:

```bash
cmake .. \
  -D32BLIT_DIR=../../32blit-sdk \
  -DPICO_SDK_PATH=../../pico-sdk \
  -DPICO_BOARD=chilichip_vgc \
  -DPICO_PLATFORM=rp2350-arm-s \
  -DCMAKE_TOOLCHAIN_FILE=../../32blit-sdk/pico2.toolchain
```

> Adjust paths if your SDKs are located elsewhere relative to your project directory.

---

## Building the Project

```bash
make
```

This produces a `.uf2` file for your game.

---

## Copying to ChiliChip VGC Zero

1. Connect the console via USB.
2. Power it off, then hold the top **X button** and press **Power**.
3. The device should appear as `RP2350` on Linux (`/media/<username>/RP2350`).
4. Copy your `.uf2` file:

```bash
cp your-game.uf2 /media/$USER/RP2350/
```

The console will automatically reboot into your game.

---

## VGC Zero OS: loader, home screen and `.blit` games

Besides standalone `.uf2` games, VGC Zero runs a small OS: the 32blit loader, a system firmware that stays in flash and installs any number of `.blit` games next to it, and the ChiliChip home screen (`vgc-os`) that lists and starts them.

### Setting up a console (factory image)

`vgc-os-factory.uf2` holds the loader with the home screen already installed, so one copy sets up a new or wiped console:

1. Put the console in BOOTSEL mode (see [Copying to ChiliChip VGC Zero](#copying-to-chilichip-vgc-zero)).
2. Copy the factory image:

```bash
cp vgc-os-factory.uf2 /media/$USER/RP2350/
```

The console restarts into the home screen. Copy `.blit` games onto its USB drive to install them (see [Installing games over USB](#installing-games-over-usb)).

The image writes the loader to flash 0–256 KB and the home screen at 256 KB, and leaves the rest of flash alone: other installed games and the USB drive (saves, settings) are kept. A game that was installed at 256 KB is overwritten and needs installing again.

The image is built along with the loader below, as `vgc-os/vgc-os-factory.uf2`, and is part of the CI's "VGC-blit" build. `tools/factory-uf2.py` makes it by adding `.blit` files at fixed flash offsets to the loader's `.uf2`, numbered as one image:

```bash
python3 tools/factory-uf2.py blit-loader.uf2 vgc-os.blit@256 -o vgc-os-factory.uf2
```

### Building the loader and `.blit` games

Build the loader, the home screen and the stock launcher from this repository:

```bash
mkdir build.vgc-blit
cd build.vgc-blit
cmake .. \
  -DPICO_SDK_PATH=../../pico-sdk \
  -DPICO_BOARD=chilichip_vgc \
  -DPICO_PLATFORM=rp2350-arm-s \
  -DCMAKE_TOOLCHAIN_FILE=../pico2.toolchain \
  -DBLIT_EXECUTABLE_PICO_BLIT=1
make
```

This produces `32blit-pico/loader/blit-loader.uf2`, `vgc-os/vgc-os.blit`, `vgc-os/vgc-os-factory.uf2` and `launcher/launcher.blit`. Build a game as `.blit` by adding `-DBLIT_EXECUTABLE_PICO_BLIT=1` to its own cmake command.

To install the parts separately, copy `blit-loader.uf2` to the console in BOOTSEL mode, then, with the console running the loader and connected over USB, install the home screen and games over USB serial:

```bash
32blit install vgc-os.blit
32blit install game.blit
```

You can also copy `.blit` files onto the console's USB drive. The launcher lists them about half a second after the PC finishes writing, and installs a game into flash when you launch it.

Hold **HOME** for one second to leave a game and return to the home screen.

### System menu, battery and settings

Press **HOME** briefly in a game to pause it and open the system menu: resume, volume, brightness, battery and quit to home. Up and down pick an item, left and right change volume and brightness, A selects and B or HOME resumes. Holding HOME quits to the home screen. Either way of quitting lets the game save first (see [Saves](#saves)). The menu doesn't open over the launcher itself.

The loader reads the battery on GPIO29 four times a second and smooths it (`32blit-pico/power.cpp`). `BATTERY_DIVIDER` in `board/chilichip_vgc/config.h` assumes the board's divider is ÷3, which is inferred from Waveshare's demos and still needs measuring against a multimeter, as does the percentage curve. "USB connected" means a computer has enumerated the console; a charger alone doesn't count.

Brightness (0–15, panel contrast), volume (0–100 %) and screen sleep are stored in `.system/settings.bin` on the USB drive and applied by the loader to every game, so games need no code for them. Standalone `.uf2` builds read the same file.

Games can read these through API 0.3 (`32blit/engine/system.hpp`): `get_battery_level()`, `get_battery_voltage()`, `is_usb_connected()` and `get_system_settings()`. `set_system_settings()` is for launchers and settings apps. A game built against API 0.3 needs a loader with API 0.3; older games keep working on the new loader.

### Screen sleep

Settings → Screen sleep turns the screen off after 1, 2, 5 or 10 minutes without a button press (Off by default). The loader dims the panel 10 seconds before the timeout, then switches it off with the SSD1351 display-off command (`32blit-pico/screen_sleep.cpp`). Any button turns it back on. While dimmed, that press also reaches the game; while the screen is off, it only wakes the screen. Games and music keep running while the screen is off, and an install in progress keeps the screen on.

Standalone `.uf2` games read the same setting. Screen sleep needs the display driver to implement `set_display_brightness()` and `set_display_power()` (`32blit-pico/display.hpp`); only the SSD1351 driver does so far.

`utilities/system-test` shows all of these on screen, and A plays a test tone to check the volume.

### Installing games over USB

Plug the console into a computer while the launcher is showing and a drive named **CHILICHIP** appears (the USB device shows as "ChiliChip Console"). Copy one or more `.blit` files to the top of the drive, then eject it, or unplug the cable. The console installs each game into flash with a progress bar, deletes the installed copies from the drive and returns to the launcher. Files still on the drive at power-on are installed the same way.

- A newer build of an installed game replaces the old one. Saves live in `.blit/<title>/` on the drive, so they are kept.
- A file that is already installed is just removed from the drive.
- Errors are shown per file and those files stay on the drive: "Not a ChiliChip game" (not a `.blit`, or built for another device), "Needs a system update" (built for a newer API than the loader), "File damaged" (truncated, or its checksum doesn't match), "Not enough space".
- While a game is running the drive is read-only, so the computer can't change files the game has open.
- Nothing is installed while the computer still has the drive mounted, because it caches the drive's contents. Until you eject, the launcher can still start a `.blit` straight from the drive.

This is enabled by `AUTO_INSTALL` in `board/chilichip_vgc/config.h` (`32blit-pico/installer.cpp`, install logic in `install_blit_file()` in `blit_launch.cpp`). The drive label comes from `STORAGE_VOLUME_LABEL` and is applied at boot, so drives formatted earlier are relabelled. `32blit install` over USB serial still works.

### Home screen (`vgc-os`)

`vgc-os/` is the console's home screen, a `.blit` with category `launcher` titled "ChiliChip OS". To update it, copy `vgc-os.blit` onto the drive like a game. The factory image installs it. Installing a launcher removes any other installed launcher, so it replaces the stock 32blit launcher (and copying `launcher.blit` back swaps them again).

- A 12 px status bar shows the battery level and a plug icon while USB is connected.
- The carousel shows one cover per installed game: the selected game's splash image large in the middle, its neighbours smaller at the edges. Left and right scroll, A plays. Games without a splash get a coloured card with their title.
- Games that have never been played come first (so new installs show up first), then the rest from most recently played. The order is kept in the home screen's save.
- Y opens game info: author, version, size and description, with Play, Delete game and Delete save data. Deleting a game that has save data then asks whether to delete the saves too; B keeps them.
- The last tile, or MENU, opens Settings: brightness, volume, screen sleep, the games list with sizes, storage used and free for games and for the USB drive, and About with the OS version, firmware version and serial number.

The system menu, the install screens and the "Deleting game" progress box are drawn by the loader with the same look as the home screen, from `launcher-shared/theme.hpp`.

Storage and About use `get_system_info()`, added in API 0.4 (`32blit/engine/system.hpp`). On desktop builds the home screen lists the `.blit` files next to the executable instead of installed games, which is handy for working on the UI.

### Saves

Games save with `blit::write_save()` and load with `blit::read_save()` (`32blit/engine/save.hpp`). Each save is a file in `.blit/<game title>/` on the USB drive, so saves survive reinstalling or updating a game, and can be backed up by copying that folder.

To save when the player quits, set `blit::on_quit` (`32blit/engine/engine.hpp`, API 0.5), usually in `init()`:

```cpp
void save_progress() {
  blit::write_save(progress);
}

void init() {
  blit::read_save(progress);
  blit::on_quit = save_progress;
}
```

The loader calls it before returning to the home screen, from the system menu's "Quit to home" or by holding HOME. Holding HOME waits for the game to finish its current frame; if a game is stuck for two more seconds, the loader quits without saving. Desktop builds call it when the window closes. The console has a hard power switch, so switching it off can't be caught: save at checkpoints too, not only on quit.

### Memory layout

| Region | VGC Zero |
| ------ | -------- |
| Flash 0–256 KB | Loader |
| Flash 256 KB–12 MB | Installed `.blit` games (64 KB blocks) |
| Flash 12–16 MB | FAT storage, shown as a USB drive (saves live here) |
| RAM 0–64 KB | Loader data, heap and the 32 KB core 0 stack that games run on |
| RAM 64–512 KB | The running game (448 KB, including its 32 KB framebuffer) |

The loader gets 64 KB of RAM instead of upstream's 32 KB so games keep the same 32 KB stack as standalone builds (`board/chilichip_vgc/memmap_blit_loader.ld`, `BLIT_LOADER_RAM_KB`). A `.blit` built against an older SDK expects games to start at 32 KB and must be rebuilt.

## ChiliChip VGC Zero Board Details

| Feature  | VGC Zero (RP2350)             |
| -------- | ----------------------------- |
| CPU      | RP2350, 150 MHz (dual-core)   |
| RAM      | 520KB + 16MB                  |
| Controls | 8 buttons + encoder           |
| Screen   | 128×128 SSD1351 OLED (SPI)    |
| Sound    | Mono speaker (PWM on GP22)    |
| Storage  | 16MB XiP QSPI                 |

The RP2350 in VGC Zero is significantly faster than the RP2040, allowing higher framerates, more complex logic, and improved audio support.

### SSD1351 display HAL

The board selects `dbi_ssd1351` (`32blit-pico/board/chilichip_vgc/config.cmake`). Pins are in `config.h`:

| Signal | GPIO |
| ------ | ---- |
| `LCD_SCK_PIN` | 18 |
| `LCD_MOSI_PIN` | 19 |
| `LCD_CS_PIN` | 17 |
| `LCD_DC_PIN` | 21 |
| `LCD_RESET_PIN` | 20 |
| SPI clock cap | 20 MHz (`LCD_MAX_CLOCK`) |
| Rotation | 2 (180°) |

There is no backlight pin. Dim the panel with `ssd1351_set_master_contrast(0…15)` (command `0xC7`), not a software black veil.

| Item | Behaviour |
| ---- | --------- |
| `0xB3` CLOCK_DIV | `0xF0` (max oscillator, ÷1) — ~2× OLED PWM refresh versus the previous `0xF1`. Override with `-DSSD1351_CLOCK_DIV=0xF1` if a panel cannot tolerate /1. |
| Init extras | `0xB2` enhance, `0xBB` precharge voltage, `0xB9` linear LUT |
| SPI | Fractional PIO clkdiv so 250 MHz sysclk actually hits **20 MHz**. Do not raise the cap to 30–40 MHz. |
| GRAM | Re-issue column/row + `WRITE_RAM` every frame (stops rolling lines from pointer drift). |
| TE | Optional `LCD_TE_PIN` / `LCD_VSYNC_PIN`. The Waveshare 7-pin module has no TE pin. |

A 128×128 RGB565 frame is 32 768 bytes ≈ **13.1 ms** at 20 MHz. Phone cameras at 30/60 fps can still beat against OLED PWM; remaining roll on a camera is the shutter, not GRAM tearing.

## References

* [32blit SDK GitHub](https://github.com/32blit/32blit-sdk)
* [32blit Examples](https://github.com/32blit/32blit-examples)
* [Pico SDK Getting Started Guide](https://datasheets.raspberrypi.org/pico/getting-started-with-pico.pdf)
