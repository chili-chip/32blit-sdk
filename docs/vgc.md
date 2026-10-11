# Building 32Blit SDK for ChiliChip VGC Zero

This guide explains how to build 32Blit SDK projects for **ChiliChip VGC Zero**, a **RP2350-based** video game console.

---

## Prerequisites

Install dependencies (Ubuntu / WSL):

```bash
sudo apt install git gcc g++ gcc-arm-none-eabi cmake make \
python3 python3-pip python3-setuptools \
libsdl2-dev libsdl2-image-dev libsdl2-net-dev unzip
```

Install the 32Blit Python tools:

```bash
python3 -m pip install --user 32blit
```

If pip installs to a directory not on PATH:

```bash
export PATH=$PATH:~/.local/bin
```

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
3. The device should appear as `RPI-RP2` on Linux (`/media/<username>/RPI-RP2`).
4. Copy your `.uf2` file:

The console will automatically reboot into your game.

---

## Loader and `.blit` games (OS preview)

Besides standalone `.uf2` games, VGC Zero can run the 32blit loader: a small system firmware that stays in flash and installs any number of `.blit` games next to it. This is the base for the VGC Zero OS.

Build the loader (and the stock launcher) from this repository:

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

This produces `32blit-pico/loader/blit-loader.uf2` and `launcher/launcher.blit`. Build a game as `.blit` by adding `-DBLIT_EXECUTABLE_PICO_BLIT=1` to its own cmake command.

To install:

1. Copy `blit-loader.uf2` to the console in BOOTSEL mode (see above).
2. With the console running the loader and connected over USB, install the launcher and games over USB serial:

```bash
32blit install launcher.blit
32blit install game.blit
```

Hold **HOME** for one second to leave a game and return to the launcher.

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
