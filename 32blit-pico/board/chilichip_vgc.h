#ifndef _BOARDS_CHILICHIP_VGC_H
#define _BOARDS_CHILICHIP_VGC_H

#define CHILICHIP_VGC

// For ChiliChip VGC board - based on RP2350 (Pico 2)
// Override default pin assignments before including base board

// --- UART ---
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
#ifndef PICO_DEFAULT_LED_PIN
#define PICO_DEFAULT_LED_PIN 25
#endif

// --- I2C ---
#ifndef PICO_DEFAULT_I2C
#define PICO_DEFAULT_I2C 1
#endif
#ifndef PICO_DEFAULT_I2C_SDA_PIN
#define PICO_DEFAULT_I2C_SDA_PIN 14
#endif
#ifndef PICO_DEFAULT_I2C_SCL_PIN
#define PICO_DEFAULT_I2C_SCL_PIN 15
#endif

// --- SPI ---
#ifndef PICO_DEFAULT_SPI
#define PICO_DEFAULT_SPI 0
#endif
#ifndef PICO_DEFAULT_SPI_SCK_PIN
#define PICO_DEFAULT_SPI_SCK_PIN 18
#endif
#ifndef PICO_DEFAULT_SPI_TX_PIN
#define PICO_DEFAULT_SPI_TX_PIN 19
#endif
#ifndef PICO_DEFAULT_SPI_RX_PIN
#define PICO_DEFAULT_SPI_RX_PIN 16
#endif
#ifndef PICO_DEFAULT_SPI_CSN_PIN
#define PICO_DEFAULT_SPI_CSN_PIN 17
#endif

// Core 1 only runs the PWM mix loop; keep it in scratch X (4 KiB).
// Must be set before PICO_STACK_SIZE or pico_multicore copies the 32 KiB value.
#ifndef PICO_CORE1_STACK_SIZE
#define PICO_CORE1_STACK_SIZE 0x800
#endif
// Core 0 parses C++ (exceptions + Room tiles). Default 2 KiB scratch Y overflows.
#ifndef PICO_STACK_SIZE
#define PICO_STACK_SIZE 0x8000
#endif

// Include the base Pico 2 board definition
#include "boards/pico2.h"

#endif
