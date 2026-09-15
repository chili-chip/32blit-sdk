#pragma once
#include <cstdint>

#include "engine/api_private.hpp"
#include "config.h"

extern blit::SurfaceInfo cur_surf_info;
extern blit::ScreenMode cur_screen_mode;

extern bool fb_double_buffer;

#if defined(BUILD_LOADER) || defined(BLIT_BOARD_PIMORONI_PICOVISION)
extern uint16_t *screen_fb;
#else
extern uint16_t screen_fb[];
#endif

int get_display_page_size();
void init_display();
void update_display(uint32_t time);

void init_display_core1();
void update_display_core1();

bool display_render_needed();

bool display_mode_supported(blit::ScreenMode new_mode, const blit::SurfaceTemplate &new_surf_template);

void display_mode_changed(blit::ScreenMode new_mode, blit::SurfaceTemplate &new_surf_template);

/// SSD1351 command 0xC7 master contrast, 0–15. Implemented by the
/// dbi_ssd1351 driver. Games can call this after init_display() to dim
/// the OLED without a software black veil (which cannot change PWM rate).
///
/// This is also the knob for horizontal banding on rows that contain bright
/// pixels: it scales segment drive current, and the banding is the row's
/// current spike sagging the panel supply. See docs/vgc.md.
void ssd1351_set_master_contrast(uint8_t level);

/// SSD1351 command 0xC1 per-colour drive current, the fine control under the
/// master contrast above. Blue is the first colour to drop out of regulation
/// when a row sags, so this is where to trim one colour rather than all three.
void ssd1351_set_contrast_abc(uint8_t a, uint8_t b, uint8_t c);

/// Re-programs the SSD1351 segment waveform registers (0xB1 phase 1/2, 0xB6
/// phase 3, 0xBB pre-charge voltage, 0xB4 VSL source). init_display() already
/// programs the SSD1351_* defaults from ssd1351_init_seq.hpp; this exists so a
/// new panel can be swept on the bench without a rebuild per value.
void ssd1351_set_row_drive(uint8_t phase_12, uint8_t phase_3, uint8_t precharge_level, uint8_t vsl_select);

blit::SurfaceInfo &set_screen_mode(blit::ScreenMode mode);
bool set_screen_mode_format(blit::ScreenMode new_mode, blit::SurfaceTemplate &new_surf_template);

void set_screen_palette(const blit::Pen *colours, int num_cols);

void set_framebuffer(uint8_t *data, uint32_t max_size, blit::Size max_bounds);
