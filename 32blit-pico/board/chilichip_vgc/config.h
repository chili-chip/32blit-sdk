#pragma once

#define PICO_RP2350 1

// D-pad buttons
#define BUTTON_LEFT_PIN  10
#define BUTTON_RIGHT_PIN 7
#define BUTTON_UP_PIN    9
#define BUTTON_DOWN_PIN  8

// Action buttons
#define BUTTON_A_PIN 3
#define BUTTON_B_PIN 5
#define BUTTON_X_PIN 6
#define BUTTON_Y_PIN 4
#define BUTTON_MENU_PIN 11
#define BUTTON_HOME_PIN 2

#define DISPLAY_WIDTH 128
#define DISPLAY_HEIGHT 128

// Speaker on GP22. Driven by the hardware PWM slice (audio/pwm.cpp), not PIO.
#define PICO_AUDIO_PWM_MONO_PIN 22
// #define AUDIO_ENABLE_PIN 15  // amp / mute GPIO, driven high at audio init

#define LED_PIN 16

#define LCD_SCK_PIN 18
#define LCD_MOSI_PIN 19
// #define LCD_BACKLIGHT_PIN 12
#define LCD_CS_PIN 17
#define LCD_DC_PIN 21
#define LCD_RESET_PIN 20

#define LCD_ROTATION 2
// SSD1351 serial-write cycle is 50 ns (20 MHz) at typical 3.3 V I/O.
// PIO SCK = clk_sys / (clkdiv * 2) and is clamped to this cap.
// 8 MHz left a 128×128 RGB565 frame on the wire for ~33 ms (~30 FPS hard
// ceiling before any game work). 20 MHz drops that to ~13 ms.
// Do not raise this to 30–40 MHz: that overclocks the panel.
#define LCD_MAX_CLOCK 20000000
// Rows containing bright pixels lose brightness because the panel supply sags
// under the row's current spike. Correct for it in the frame; costs a 32 KiB
// scratch buffer and one pass over the framebuffer per update. See docs/vgc.md
// and ssd1351_set_row_compensation() for tuning it on the panel.
#define SSD1351_ROW_COMPENSATION 1
// Waveshare 1.5" SSD1351 modules do not break out TE. Define LCD_TE_PIN
// or LCD_VSYNC_PIN if a later board wires it; the driver then waits on
// the rising edge before DMA, same as the ST7789 HAL.
// #define LCD_TE_PIN 12

// #define LED_INVERTED
// #define LED_R_PIN 6
// #define LED_G_PIN 7
// #define LED_B_PIN 8