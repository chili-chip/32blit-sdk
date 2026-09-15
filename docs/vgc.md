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
| Row banding | Supply droop under a lit row's current. Corrected in the frame by `SSD1351_ROW_COMPENSATION` — see below |
| Segment waveform | `0xB1`/`0xB6` pre-charge periods, `0xBB` pre-charge voltage, `0xB4` VSL source — see below |
| SPI | Fractional PIO clkdiv so 250 MHz sysclk actually hits **20 MHz**. Do not raise the cap to 30–40 MHz. |
| GRAM | Re-issue column/row + `WRITE_RAM` every frame (stops rolling lines from pointer drift). |
| TE | Optional `LCD_TE_PIN` / `LCD_VSYNC_PIN`. The Waveshare 7-pin module has no TE pin. |

A 128×128 RGB565 frame is 32 768 bytes ≈ **13.1 ms** at 20 MHz. Phone cameras at 30/60 fps can still beat against OLED PWM; remaining roll on a camera is the shutter, not GRAM tearing.

#### Horizontal banding on rows with bright pixels

Symptom: a solid dark background picks up a horizontal band across every row
that contains bright pixels somewhere along it, extending the full width of the
row, well to the left and right of those pixels.

Measured off a photo of the panel, on three background strips 10, 21 and 32
columns in from the left edge, against rows lit by a UI box several hundred
columns away. All three strips dip by the same amount on the same rows:

* background luminance **down about 8–9%** on the most heavily lit rows,
* red **up about 3% relative to blue** — blue and green lose more than red,
* no gradient across the row: the dip is the same near the edge and a third of
  the way in.

That is supply droop, not a timing or reference setting. The SSD1351 drives a
passive matrix, so one COM is selected at a time and every lit segment in that
row sinks its current through that one COM electrode; a mostly lit row pulls a
far bigger current spike than a row of dark background, and the panel supply
sags under it. Every pixel in that row is then driven with less than its
programmed current, and blue suffers most because it has the highest forward
voltage and so the least headroom to give up — hence the red shift.

The loss turns out to be proportional to the row's current rather than a
threshold the supply either clears or does not, which the contrast sweep below
is what establishes.

**The real fix is on the board:** VCC and VCOMH decoupling at the panel, per the
datasheet application circuit, and how both are routed to the FPC. Verify by
scoping VCC at the panel while a bright bar is on screen; the band is the ripple
at row rate.

**The contrast registers cannot fix it, and that has been confirmed on the
panel.** `0xC7` scales the drive current of every pixel by the same factor, so
the sag and the background's own brightness come down together and the
*percentage* dip lands exactly where it started. Stepping it from `0x0F` down
changes the band not at all, which is both the confirmation that the droop is
proportional and the reason `0xC7` stays at maximum here — dimming the panel
would buy nothing. Only something that changes bright pixels relative to dark
ones moves the band, which is what the compensation pass below does.

`0xC1` per-colour current (`SSD1351_CONTRAST_A`/`_B`/`_C`, default
`0xC8`/`0x80`/`0xC8`, and `ssd1351_set_contrast_abc()` at runtime) is the one
contrast control that can change one colour relative to the others, so it is
worth a try if a *hue* band remains after the compensation below. Blue has the
highest forward voltage and so the least headroom when a row sags.

#### Row-load compensation

Grey scale on this controller is pulse width, not amplitude, so a pixel starved
of current can be given the charge back as time. `SSD1351_ROW_COMPENSATION`,
enabled for this board in `config.h`, scales every pixel in a row up in
proportion to that row's drive current, which puts the band back flat without
dimming anything.

| Define | Default | Notes |
| ------ | ------- | ----- |
| `SSD1351_ROW_COMPENSATION` | set for this board | Turns the pass on. Costs a 32 KiB scratch frame and one pass over the framebuffer per update. |
| `SSD1351_ROW_COMPENSATION_R`/`_G`/`_B` | `20` | Percent gain applied to a fully lit row. `20` is what cancels the measured 8.4% dip across the whole load range. |

Tune it on the panel with `ssd1351_set_row_compensation(r, g, b)`; `0, 0, 0` is
a pass-through, for an A/B against the uncorrected frame. Raise the numbers
together until the band flattens, then split them if a hue band remains — the
photo suggests red wants a few points less than green and blue, but that part
of the measurement came through a camera's colour matrix, so it is not baked in.

Implementation notes:

* The correction at background levels is a fraction of a level, so the pass
  carries the remainder along each row rather than rounding it away; the
  fraction comes out as that proportion of the row's pixels moving up one
  level. Starting remainders are staggered per row, or every row would round up
  at the same columns and the correction would land as vertical stripes.
* It writes a scratch frame rather than working in place, so a game that only
  redraws part of the screen does not accumulate the correction.
* `blit-loader` is confined to the bottom 32 KiB of RAM and has no room for the
  scratch frame, so it runs uncorrected. Games flashed as their own `.uf2`, the
  workflow above, are unaffected.
* Odd rotations put the framebuffer's rows down the panel's columns, so the pass
  skips itself rather than correcting the wrong axis.
* The pass runs where `update()` runs, which is the main loop on this board.
  Wiring `LCD_TE_PIN` would move `update()` into a GPIO handler, so that
  combination is a compile error rather than a surprise.

`ssd1351_row_compensation_test.cpp` runs a synthetic frame through a model of
the droop fitted to the measurement and checks the pass flattens it, that zero
strength is a pass-through, that black stays black and white does not wrap, and
that the correction is not landing in stripes.

#### Segment waveform settings

`0xB1` phase 1/2, `0xB6` phase 3, `0xBB` pre-charge voltage and `0xB4` VSL
source are all overridable (`SSD1351_PHASE_12`, `SSD1351_PRECHARGE_2`,
`SSD1351_PRECHARGE_LEVEL`, `SSD1351_VSL_SELECT`) and
`ssd1351_set_row_drive(phase_12, phase_3, precharge_level, vsl_select)` writes
all four at runtime.

These were tried against the banding above and made no difference to it, which
is consistent with the supply-droop explanation, so they stay on the values the
panel has been running — the reference-init values, which are the minimum for
phases 2 and 3 and the maximum for pre-charge voltage. The power-on values
instead are `0x82` / `0x08` / `0x17`; phases 2 and 3 at the power-on periods
cost about 8% of refresh rate, since a row period is phase 1 + phase 2 + drive
DCLKs. They are the settings worth sweeping for any *other* segment artefact:
smearing or ghosting wants longer phases, dark pixels glowing wants a lower
pre-charge voltage.

## References

* [32blit SDK GitHub](https://github.com/32blit/32blit-sdk)
* [32blit Examples](https://github.com/32blit/32blit-examples)
* [Pico SDK Getting Started Guide](https://datasheets.raspberrypi.org/pico/getting-started-with-pico.pdf)
