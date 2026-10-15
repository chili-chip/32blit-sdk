# VGC Zero

The **ChiliChip VGC Zero** is a handheld console built on the Raspberry Pi RP2350. It runs ChiliChip OS, which installs games as `.blit` files and starts them from a home screen.

## Hardware

| Part | VGC Zero |
| ---- | -------- |
| Processor | RP2350, dual Arm Cortex-M33, run at 250 MHz by the SDK |
| RAM | 520 KB on chip; 448 KB of it is available to a running game |
| Flash | 16 MB QSPI: OS, installed games and a 4 MB USB drive |
| Screen | 1.5" 128×128 SSD1351 colour OLED over SPI, 16-bit colour |
| Sound | mono speaker, driven by hardware PWM on GPIO 22 |
| Controls | d-pad, A, B, X, Y, MENU and HOME buttons |
| Light | one status LED on GPIO 16 (on or off) |
| Power | battery with level sensing on GPIO 29; a hard power switch |
| USB | USB-C: the console's drive, serial for `32blit install`, and charging |

There is no real-time clock, no tilt sensor and no analog stick, so `tilt` and `joystick` stay at zero on the console.

## Buttons

| Button | In your game | Reserved by the OS |
| ------ | ------------ | ------------------ |
| D-pad | `DPAD_UP`, `DPAD_DOWN`, `DPAD_LEFT`, `DPAD_RIGHT` | |
| A, B, X, Y | `A`, `B`, `X`, `Y` | |
| MENU | `MENU` | |
| HOME | (`HOME`, but don't use it) | short press: system menu; hold one second: quit to home |

HOME belongs to the OS. Your game can still see it in `buttons` while it is held, but releasing it opens the system menu over your game, so don't give it a job of your own. Use MENU for your own pause menu. (In a standalone `.uf2` build there is no OS, and HOME is an ordinary button.)

Holding the top **X** button while switching on enters the RP2350's BOOTSEL mode, used to flash `.uf2` files (see [Flashing](flashing.md)).

## Screen

The screen is 128×128 pixels. `set_screen_mode(ScreenMode::hires)` gives you all of it; `ScreenMode::lores` (the default) gives a 64×64 framebuffer that the display driver doubles up to fill the screen, using a quarter of the memory.

| Mode | Size | Format | Notes |
| ---- | ---- | ------ | ----- |
| `lores` | 64×64 | RGB565 | the default at start-up; each pixel is shown as 2×2 |
| `hires` | 128×128 | RGB565 | the whole screen; a 32 KB framebuffer |

`hires_palette` isn't supported on the VGC Zero: the display driver only accepts RGB565, so `set_screen_mode(ScreenMode::hires_palette)` leaves the screen as it was. (It works in desktop builds, which can make the difference easy to miss.)

A full 128×128 frame takes about 13 ms to send to the panel at the 20 MHz SPI clock, so the screen can refresh well above 30 frames a second. The [display driver](display.md) page has the details.

The OLED has no backlight: brightness is the panel's contrast setting, which the player controls from the system menu.

## Sound

`blit::channels` gives you 8 synth channels mixed at 22 050 Hz, played through a PWM output on GPIO 22. Mixing runs on the second core from RAM, so sound doesn't stall while the game reads flash or sends the screen. The player's volume setting is applied on top of yours. See [Audio](../api/audio.md).

## More about the console

* [ChiliChip OS](chilichip-os.md): the home screen, installing games, the system menu, settings and screen sleep.
* [Flashing and the loader](flashing.md): setting up a console, the factory image, and building the OS yourself.
* [Storage, saves and memory](storage.md): where games, saves and settings live, and how memory is split.
* [Display driver](display.md): the SSD1351 driver and its tuning.
