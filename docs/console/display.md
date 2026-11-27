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

On the OLED there is no backlight. Brightness is the panel's master contrast (command `0xC7`, levels 0–15), set by the loader from the player's setting. Don't dim the screen by drawing a translucent black layer: it costs time every frame and looks worse.

[Screen sleep](chilichip-os.md#screen-sleep) uses the driver's `set_display_brightness()` and `set_display_power()` (the SSD1351 display-off command), declared in `32blit-pico/display.hpp`. The SSD1351 driver implements both; the ST7789 driver below implements `set_display_power()`.

## 2.0" TFT (ER-TFT020-3)

The bigger screen is an [EastRising ER-TFT020-3](https://www.buydisplay.com/2-inch-240x320-ips-tft-lcd-display-with-connector-fpc), a 2.0" IPS 320×240 ST7789 panel, mounted sideways on the TFT display module. It uses the same pins as the OLED. Build for it with:

```sh
cmake ... -DPICO_BOARD=chilichip_vgc -DBLIT_DISPLAY_DRIVER=dbi_st7789_scaled
```

Games still draw into a 128×128 framebuffer, so every game runs unchanged. The `dbi_st7789_scaled` driver scales each frame up and centres it:

| Game mode | On the panel | Scale |
| --------- | ------------ | ----- |
| hires 128×128 | 240×240, 40 px black bars left and right | 1.875× |
| lores 64×64 | 240×240 | 3.75× |

The scaling is nearest neighbour, so pixels stay sharp. At 1.875× most source pixels are 2 panel pixels wide and every eighth one is 1 pixel wide. The driver fills one 240-pixel line in the DMA interrupt, only when the source row changes, and sends it by DMA.

| Item | Setting |
| ---- | ------- |
| SPI clock | 62.5 MHz cap (`ST7789_MAX_CLOCK`), about 15 ms per frame |
| Rotation | `ST7789_ROTATION`, 90 (default) or 270, to suit how the panel is mounted |
| Colour order | `ST7789_BGR` 1 if red and blue are swapped |
| Tearing effect | the panel has no TE pin, so frames are sent every 20 ms |
| Backlight | on the module, switched on by the reset line. No dimming yet. |
| Screen sleep | `set_display_power()` uses the ST7789 sleep and display-off commands |

The scale helpers are in `32blit-pico/display/scale_map.hpp`. Check them on the host with:

```sh
cd 32blit-pico/display
g++ -std=c++17 -o /tmp/scale_map_test scale_map_test.cpp && /tmp/scale_map_test
```

Not done yet: picking the driver at run time from the panel's ID ([#108](https://github.com/chili-chip/32blit-sdk/issues/108)), and letting new games ask for the full 320×240.
