#pragma once

// ChiliChip console main board v3.0.0 with the 1.44" ST7735S TFT
// (vgc-zero-hardware #41). Same pins as the OLED version.

// ST7735S serial write cycle is 66 ns (15 MHz).
#define LCD_MAX_CLOCK 15000000
// Not checked on a real panel yet: set once the carrier board exists.
#define LCD_ROTATION 0

// The backlight follows the display's RST line (vgc-zero-hardware #41),
// so there is no brightness control and no LCD_BACKLIGHT_PIN.

#include "../chilichip_vgc/config.h"
