#ifndef _BOARDS_CHILICHIP_VGC_TFT_H
#define _BOARDS_CHILICHIP_VGC_TFT_H

// ChiliChip console main board v3.0.0 with the TFT display. Same pins as
// chilichip_vgc, so this only repeats the lines CMake reads from this file.
pico_board_cmake_set(PICO_PLATFORM, rp2350-arm-s)
pico_board_cmake_set_default(PICO_FLASH_SIZE_BYTES, (16 * 1024 * 1024))

#define CHILICHIP_VGC_TFT

#include "chilichip_vgc.h"

#endif
