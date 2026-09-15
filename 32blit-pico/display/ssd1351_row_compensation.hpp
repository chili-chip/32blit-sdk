#pragma once

#include <cstdint>

/// Row-load compensation for the SSD1351.
///
/// The panel loses brightness on rows that contain bright pixels: it drives a
/// passive matrix, one COM at a time, so a mostly lit row pulls a much bigger
/// current spike through its one COM electrode than a row of dark background,
/// and the panel supply sags under it. Every pixel in the row comes out dimmer,
/// uniformly across the full width, which reads as a horizontal band.
///
/// Grey scale on this controller is pulse width rather than amplitude, so a
/// pixel starved of current can be given the charge back as time. Scaling every
/// pixel in a row up in proportion to that row's drive current puts the band
/// back flat.
///
/// Note this cannot be done with the contrast registers. They scale the drive
/// current of every pixel by the same factor, which scales the sag and the
/// background's own brightness together and leaves the *percentage* dip where
/// it was. Only something that changes bright pixels relative to dark ones can
/// move it, which is why this works and 0xC7 does not.
///
/// Gain applied to a fully lit row, per channel, in percent.
///
/// 20 comes out of the measurement in docs/vgc.md: the rows carrying a UI box
/// lost 8.4% of their background, and a gain of 1 + 0.20 * row_load cancels
/// that to within a few tenths of a percent over the whole load range. Red
/// wants a few points less than green and blue, but that part of the
/// measurement came through a camera's colour matrix, so it is left to
/// ssd1351_set_row_compensation() to trim on the panel.
#ifndef SSD1351_ROW_COMPENSATION_R
#define SSD1351_ROW_COMPENSATION_R 20
#endif
#ifndef SSD1351_ROW_COMPENSATION_G
#define SSD1351_ROW_COMPENSATION_G 20
#endif
#ifndef SSD1351_ROW_COMPENSATION_B
#define SSD1351_ROW_COMPENSATION_B 20
#endif

inline constexpr uint8_t kSsd1351RowCompensationDefault[3] = {
  SSD1351_ROW_COMPENSATION_R, SSD1351_ROW_COMPENSATION_G, SSD1351_ROW_COMPENSATION_B
};

/// `pct` is the gain applied to a fully lit row, per channel, in percent.
/// Row length is capped at 256 so the load accumulator cannot overflow the
/// fixed-point gain below.
inline void ssd1351_compensate_rows(const uint16_t *src, uint16_t *dst, int w, int h,
                                    const uint8_t pct[3]) {
  // Per-pixel contribution to the row's drive current, r + g/2 + b in 5-bit
  // steps, so a fully lit row is w * 93.
  const uint32_t full = static_cast<uint32_t>(w) * 93u;

  // gain = 1 + pct * load / full, 8.8 fixed point
  uint32_t k[3];
  for(int c = 0; c < 3; c++)
    k[c] = (static_cast<uint32_t>(pct[c]) * 256u * 65536u) / (100u * full);

  for(int y = 0; y < h; y++) {
    const uint16_t *s = src + y * w;
    uint16_t *d = dst + y * w;

    uint32_t load = 0;
    for(int x = 0; x < w; x++) {
      const uint16_t p = s[x];
      load += (p >> 11) + ((p >> 6) & 0x1F) + (p & 0x1F);
    }

    const uint32_t gain_r = 256 + ((load * k[0]) >> 16);
    const uint32_t gain_g = 256 + ((load * k[1]) >> 16);
    const uint32_t gain_b = 256 + ((load * k[2]) >> 16);

    if((gain_r | gain_g | gain_b) == 256) { // nothing to correct, e.g. a black row
      for(int x = 0; x < w; x++)
        d[x] = s[x];
      continue;
    }

    // At background levels a row's correction is a fraction of a level, and
    // rounding it away would put back a band of its own. Carry the remainder
    // along the row instead, so the row's mean comes out exact and the
    // fraction shows up as that proportion of its pixels moving up one level.
    // The starting remainders are staggered per row to keep the pattern from
    // lining up into vertical stripes.
    uint32_t err_r = (y * 97) & 0xFF;
    uint32_t err_g = (y * 149) & 0xFF;
    uint32_t err_b = (y * 211) & 0xFF;

    for(int x = 0; x < w; x++) {
      const uint16_t p = s[x];

      uint32_t v = (p >> 11) * gain_r + err_r;
      uint32_t r = v >> 8;
      err_r = v & 0xFF;
      if(r > 0x1F) { r = 0x1F; err_r = 0; }

      v = ((p >> 5) & 0x3F) * gain_g + err_g;
      uint32_t g = v >> 8;
      err_g = v & 0xFF;
      if(g > 0x3F) { g = 0x3F; err_g = 0; }

      v = (p & 0x1F) * gain_b + err_b;
      uint32_t b = v >> 8;
      err_b = v & 0xFF;
      if(b > 0x1F) { b = 0x1F; err_b = 0; }

      d[x] = static_cast<uint16_t>((r << 11) | (g << 5) | b);
    }
  }
}
