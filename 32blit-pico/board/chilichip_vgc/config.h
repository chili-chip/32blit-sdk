#pragma once

// ChiliChip console main board v3.0.0.
// Pin map: https://hardware.chilichip.eu/console/pin-map/
// chilichip_vgc_tft includes this file after its display settings.

#define PICO_RP2350 1

// Buttons module. Active low with the internal pull-ups.
#define BUTTON_UP_PIN    19
#define BUTTON_DOWN_PIN  16
#define BUTTON_LEFT_PIN  20
#define BUTTON_RIGHT_PIN 17

#define BUTTON_A_PIN 7
#define BUTTON_B_PIN 8
#define BUTTON_X_PIN 11
#define BUTTON_Y_PIN 10

// The board has Start and Select, not MENU and HOME.
// Start = MENU: the game's pause button. Hold it to sleep.
// Select = HOME: opens the system menu, hold it to quit to the home screen.
#define BUTTON_START_PIN  18
#define BUTTON_SELECT_PIN 12
#define BUTTON_MENU_PIN BUTTON_START_PIN
#define BUTTON_HOME_PIN BUTTON_SELECT_PIN

#define DISPLAY_WIDTH 128
#define DISPLAY_HEIGHT 128

// Audio module: MAX98357A over I2S. LRCLK must be BCLK + 1.
// The display uses PIO0, so audio goes on PIO1.
#define AUDIO_I2S_DATA_PIN 3
#define AUDIO_I2S_CLOCK_PIN_BASE 4
#define AUDIO_I2S_PIO 1
// AUDIO_MODE: high turns the amplifier on (left + right mix), low shuts it down
#define AUDIO_ENABLE_PIN 6

// Radio module (RM2): WL_ON and BT_ON. Kept low until there is a driver.
#define RADIO_POWER_PIN 9

// Utility module on I2C0 (GP0 / GP1). The loader looks for its parts at
// start-up (modules.cpp).
#define DEFAULT_I2C_CLOCK 400000
#define UTILITY_MODULE_I2C 1

// Battery sense on the Waveshare RP2350-Plus. Per the schematic GPIO29 reads
// VSYS through a 200K/100K divider (R1/R7), and VSYS is fed from the battery
// through a MBR230LSFT1G Schottky (D2), so add its ~0.3 V drop back to get
// the battery voltage. Without it a full 4.2 V cell read ~3.9 V (63%).
#define BATTERY_ADC_PIN 29
#define BATTERY_DIVIDER 3.0f
#define BATTERY_OFFSET_MV 300

// No power switch: hold Start to sleep and to wake (sleep.cpp)
#define SLEEP_BUTTON_PIN BUTTON_START_PIN
// go to sleep below this battery voltage, before the cell's protection cuts out
#define SLEEP_BATTERY_MV 3450

// Short press of HOME in a game opens the system menu (loader only)
#define SYSTEM_MENU 1

// .blit files copied onto the USB drive install on the home screen once the
// drive is ejected, and the drive is read-only while a game runs (loader only)
#define AUTO_INSTALL 1
#define CONSOLE_NAME "ChiliChip"

// USB device name ("ChiliChip Console") and drive label (FAT allows 11 characters)
#define USB_VENDOR_STR "ChiliChip"
#define USB_PRODUCT_STR "Console"
#define STORAGE_VOLUME_LABEL "CHILICHIP"

// Display header. Driven by PIO0, so SPI1 itself is unused.
#define LCD_SCK_PIN 26
#define LCD_MOSI_PIN 27
#define LCD_CS_PIN 28
#define LCD_DC_PIN 22
#define LCD_RESET_PIN 21

#ifndef LCD_ROTATION
#define LCD_ROTATION 2
#endif

#ifndef LCD_MAX_CLOCK
// SSD1351 serial-write cycle is 50 ns (20 MHz) at typical 3.3 V I/O.
// PIO SCK = clk_sys / (clkdiv * 2) and is clamped to this cap.
// 8 MHz left a 128×128 RGB565 frame on the wire for ~33 ms (~30 FPS hard
// ceiling before any game work). 20 MHz drops that to ~13 ms.
// Do not raise this to 30–40 MHz: that overclocks the panel.
#define LCD_MAX_CLOCK 20000000
#endif
// Neither display module breaks out TE, and there is no free pin for it.
