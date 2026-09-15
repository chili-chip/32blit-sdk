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
| Init extras | `0xB2` enhance, `0xB9` linear LUT |
| Row drive | `0xB4` internal VSL, `0xB1`/`0xB6` pre-charge periods, `0xBB` pre-charge voltage — see below |
| SPI | Fractional PIO clkdiv so 250 MHz sysclk actually hits **20 MHz**. Do not raise the cap to 30–40 MHz. |
| GRAM | Re-issue column/row + `WRITE_RAM` every frame (stops rolling lines from pointer drift). |
| TE | Optional `LCD_TE_PIN` / `LCD_VSYNC_PIN`. The Waveshare 7-pin module has no TE pin. |

A 128×128 RGB565 frame is 32 768 bytes ≈ **13.1 ms** at 20 MHz. Phone cameras at 30/60 fps can still beat against OLED PWM; remaining roll on a camera is the shutter, not GRAM tearing.

#### Row cross-talk (tinted horizontal bands)

Symptom: a solid dark background picks up a tinted horizontal line across every
row that contains bright pixels somewhere along it, extending to the left and
right of those pixels.

The SSD1351 drives a passive matrix, so one COM (row) is selected at a time and
every lit segment in that row sinks its current through that single COM
electrode. Anything that lets the row's shared references move with the row's
total current makes the rest of that row shift brightness and hue. The init
table therefore uses the datasheet power-on values for the four registers that
decide how much of this the driver generates, all overridable from a board
`config.cmake` or the cmake command line:

| Define | Default | Notes |
| ------ | ------- | ----- |
| `SSD1351_VSL_SELECT` | `0xA2` (internal VSL) | External VSL (`0xA0`) needs a resistor and diode from the VSL pin to VSS (datasheet figure 14-1). The `0xB4` note says that circuit is required "in order to avoid distortion in display pattern"; with the pin left open there is nothing holding the segment reference. |
| `SSD1351_PHASE_12` | `0x82` | Phase 1 = 5 DCLK, phase 2 (first pre-charge) = 8 DCLK. |
| `SSD1351_PRECHARGE_2` | `0x08` | Phase 3 (second pre-charge) = 8 DCLK. |
| `SSD1351_PRECHARGE_LEVEL` | `0x17` | About 0.47 x VCC. `0x1F` is the 0.60 x VCC maximum, which parks dark pixels just under their turn-on point. |
| `SSD1351_CONTRAST_A`/`_B`/`_C` | `0xC8`/`0x80`/`0xC8` | Lowering these lowers the current a bright row pulls through its COM electrode. Costs brightness, so it is the last knob to reach for. |

Phases 2 and 3 were previously at their minimums (`0x32` / `0x01`) to shave a
row period. That leaves the segment current sources still charging pixel
capacitance once the constant-current stage starts, so a pixel's final
brightness depends on how loaded the rest of its row is. A row period is
phase 1 + phase 2 + drive DCLKs, so the power-on periods give back about 8% of
refresh rate, which the `0xF0` clock divider above already more than covers.

If bands remain after all of that, it is the panel supply rather than the
driver: `VCC` and `VCOMH` sag under the extra row current. Check local
decoupling on both and how they are routed to the FPC.

## References

* [32blit SDK GitHub](https://github.com/32blit/32blit-sdk)
* [32blit Examples](https://github.com/32blit/32blit-examples)
* [Pico SDK Getting Started Guide](https://datasheets.raspberrypi.org/pico/getting-started-with-pico.pdf)
