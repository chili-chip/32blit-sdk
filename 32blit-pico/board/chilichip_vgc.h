#ifndef _BOARDS_CHILICHIP_VGC_H
#define _BOARDS_CHILICHIP_VGC_H

#define CHILICHIP_VGC

// The SDK only reads this from the board header named by PICO_BOARD, not from
// pico2.h below, so without it a build that doesn't pass -DPICO_PLATFORM
// silently targets the RP2040.
pico_board_cmake_set(PICO_PLATFORM, rp2350-arm-s)

// ChiliChip console main board v3.0.0 with a Waveshare RP2350-Plus.
// Pin map: https://hardware.chilichip.eu/console/pin-map/
// Board v1.1.4 is chilichip_vgc_v1.

// --- UART ---
// GP0 / GP1 are the utility module's I2C. The loader still enables stdio on
// UART0, but init_i2c() takes the pins back after it. Use USB serial for debug.
#ifndef PICO_DEFAULT_UART
#define PICO_DEFAULT_UART 0
#endif
#ifndef PICO_DEFAULT_UART_TX_PIN
#define PICO_DEFAULT_UART_TX_PIN 0
#endif
#ifndef PICO_DEFAULT_UART_RX_PIN
#define PICO_DEFAULT_UART_RX_PIN 1
#endif

// --- LED ---
// The RP2350-Plus LED. The main board has none.
#ifndef PICO_DEFAULT_LED_PIN
#define PICO_DEFAULT_LED_PIN 25
#endif

// --- I2C ---
// Utility module: IMU, RTC and haptic driver.
#ifndef PICO_DEFAULT_I2C
#define PICO_DEFAULT_I2C 0
#endif
#ifndef PICO_DEFAULT_I2C_SDA_PIN
#define PICO_DEFAULT_I2C_SDA_PIN 0
#endif
#ifndef PICO_DEFAULT_I2C_SCL_PIN
#define PICO_DEFAULT_I2C_SCL_PIN 1
#endif

// --- SPI ---
// Display. Driven by PIO, the SPI1 numbers are for reference.
#ifndef PICO_DEFAULT_SPI
#define PICO_DEFAULT_SPI 1
#endif
#ifndef PICO_DEFAULT_SPI_SCK_PIN
#define PICO_DEFAULT_SPI_SCK_PIN 26
#endif
#ifndef PICO_DEFAULT_SPI_TX_PIN
#define PICO_DEFAULT_SPI_TX_PIN 27
#endif
#ifndef PICO_DEFAULT_SPI_CSN_PIN
#define PICO_DEFAULT_SPI_CSN_PIN 28
#endif

// Core 1 only runs the audio mix loop; keep it in scratch X (4 KiB).
// Must be set before PICO_STACK_SIZE or pico_multicore copies the 32 KiB value.
#ifndef PICO_CORE1_STACK_SIZE
#define PICO_CORE1_STACK_SIZE 0x800
#endif
// Core 0 parses C++ (exceptions + Room tiles). Default 2 KiB scratch Y overflows.
#ifndef PICO_STACK_SIZE
#define PICO_STACK_SIZE 0x8000
#endif

// --- FLASH ---
// Waveshare RP2350-Plus 16MB. Without this the pico2.h default (4 MB)
// applies, which hides 12 MB from the game area and puts the storage drive
// at the wrong offset.
pico_board_cmake_set_default(PICO_FLASH_SIZE_BYTES, (16 * 1024 * 1024))
#ifndef PICO_FLASH_SIZE_BYTES
#define PICO_FLASH_SIZE_BYTES (16 * 1024 * 1024)
#endif

// Include the base Pico 2 board definition
#include "boards/pico2.h"

#endif
