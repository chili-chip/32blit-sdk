#pragma once

#include <cstdint>

// RP2350 QMI address translation maps a window of up to 4MB in 4k pages.
// Blits are linked at a 4MB offset and mapped onto wherever they are installed.
constexpr uint32_t flash_translation_page = 4 * 1024;
constexpr uint32_t flash_translation_max_size = 4 * 1024 * 1024;

// How much of the window to map for a blit of blit_size bytes.
// 0 (unknown size) maps the whole window.
constexpr uint32_t flash_translation_size(uint32_t blit_size) {
  if(blit_size == 0 || blit_size >= flash_translation_max_size)
    return flash_translation_max_size;

  return (blit_size + flash_translation_page - 1) & ~(flash_translation_page - 1);
}
