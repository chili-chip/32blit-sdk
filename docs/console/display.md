# Display driver

The VGC Zero uses a 128×128 SSD1351 colour OLED over SPI. The board selects the `dbi_ssd1351` driver in `32blit-pico/board/chilichip_vgc/config.cmake`, and the pins are set in `config.h`:

| Signal | GPIO |
| ------ | ---- |
| `LCD_SCK_PIN` | 18 |
| `LCD_MOSI_PIN` | 19 |
| `LCD_CS_PIN` | 17 |
| `LCD_DC_PIN` | 21 |
| `LCD_RESET_PIN` | 20 |
| SPI clock cap | 20 MHz (`LCD_MAX_CLOCK`) |
| Rotation | 2 (180°, `LCD_ROTATION`) |

## Timing

A 128×128 RGB565 frame is 32 768 bytes, which takes about **13.1 ms** to send at 20 MHz. The transfer runs by DMA, so the CPU is free to work on the next frame meanwhile. That leaves room for well over 30 frames a second.

Phone cameras at 30 or 60 fps can still beat against the OLED's own PWM refresh. Rolling bands on a camera are the shutter, not tearing in the picture.

## Tuning

| Item | Setting |
| ---- | ------- |
| `0xB3` clock divider | `0xF0`: maximum oscillator, ÷1, about twice the OLED's PWM refresh of the old `0xF1`. Build with `-DSSD1351_CLOCK_DIV=0xF1` if a panel can't handle ÷1. |
| Extra init | `0xB2` enhance, `0xBB` precharge voltage, `0xB9` linear grey-scale table |
| SPI clock | a fractional PIO clock divider, so the 250 MHz system clock really gives 20 MHz. Don't raise the cap to 30–40 MHz: that overclocks the panel. |
| Frame writes | the column and row window and `WRITE_RAM` are re-sent every frame, which stops rolling lines from the panel's address pointer drifting |
| Tearing effect | optional `LCD_TE_PIN` or `LCD_VSYNC_PIN`. The current module has no TE pin; if a later board wires one, the driver waits for it before each transfer. |

## Brightness and power

There is no backlight. Brightness is the panel's master contrast (command `0xC7`, levels 0–15), set by the loader from the player's setting. Don't dim the screen by drawing a translucent black layer: it costs time every frame and looks worse.

[Screen sleep](chilichip-os.md#screen-sleep) uses the driver's `set_display_brightness()` and `set_display_power()` (the SSD1351 display-off command), declared in `32blit-pico/display.hpp`. Only the SSD1351 driver implements them so far.
