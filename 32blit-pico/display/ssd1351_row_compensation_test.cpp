#include "ssd1351_row_compensation.hpp"

#include <cstdio>
#include <cstdlib>
#include <vector>

// Host-only check of the row-load compensation pass. Not part of BlitHalPico.
//   g++ -std=c++17 -o /tmp/ssd1351_row_compensation_test ssd1351_row_compensation_test.cpp
//   /tmp/ssd1351_row_compensation_test
//
// Runs a synthetic frame through a model of the panel's droop and checks that
// the pass flattens the band it produces. The droop model is the measurement
// in docs/vgc.md: output scales by (1 - loss * row_load / full_row), with loss
// fitted so a row half filled with white loses the 8.4% measured off the panel.
// That the shipped strength cancels this is only as good as the model, but it
// does check the fixed point, the dither, the clamping and that the default is
// coherent with the measurement rather than guessed.

static const int W = 128, H = 128;

static uint16_t rgb565(int r, int g, int b) {
  return static_cast<uint16_t>((r << 11) | (g << 5) | b);
}

static uint32_t row_load(const uint16_t *row) {
  uint32_t load = 0;
  for(int x = 0; x < W; x++) {
    const uint16_t p = row[x];
    load += (p >> 11) + ((p >> 6) & 0x1F) + (p & 0x1F);
  }
  return load;
}

/// What the panel emits for one row, in arbitrary linear light units, summed
/// over a span of background pixels. Grey scale is pulse width, so emitted
/// light is the pixel's level times whatever fraction of its programmed
/// current the sagging supply actually delivered.
static double emitted_blue(const uint16_t *row, int x0, int x1, double loss) {
  const double sag = 1.0 - loss * double(row_load(row)) / double(W * 93);
  double sum = 0;
  for(int x = x0; x < x1; x++)
    sum += (row[x] & 0x1F) * sag;
  return sum / (x1 - x0);
}

int main() {
  // Dark blue background with a white bar across the middle third of rows,
  // spanning half the width and nowhere near the sampled background strip.
  std::vector<uint16_t> frame(W * H, rgb565(0, 0, 8));
  for(int y = 48; y < 80; y++)
    for(int x = 64; x < W; x++)
      frame[y * W + x] = rgb565(0x1F, 0x3F, 0x1F);

  // 0.18 loss at a fully lit row puts a half filled row at the measured 8.5%.
  const double loss = 0.18;

  std::vector<uint16_t> corrected(W * H);
  ssd1351_compensate_rows(frame.data(), corrected.data(), W, H, kSsd1351RowCompensationDefault);

  // Background strip well clear of the bar, as in the photo measurement.
  const int x0 = 4, x1 = 36;
  const double plain_dark = emitted_blue(&frame[16 * W], x0, x1, loss);
  const double plain_lit  = emitted_blue(&frame[64 * W], x0, x1, loss);
  const double comp_dark  = emitted_blue(&corrected[16 * W], x0, x1, loss);
  const double comp_lit   = emitted_blue(&corrected[64 * W], x0, x1, loss);

  const double plain_band = (plain_lit / plain_dark - 1.0) * 100.0;
  const double comp_band  = (comp_lit / comp_dark - 1.0) * 100.0;

  std::printf("band in the background strip: %+.2f%% uncorrected, %+.2f%% corrected\n",
              plain_band, comp_band);

  if(plain_band > -6.0) {
    std::fprintf(stderr, "droop model is not reproducing the measured band (%+.2f%%)\n", plain_band);
    return 1;
  }
  if(std::abs(comp_band) > std::abs(plain_band) / 4.0) {
    std::fprintf(stderr, "compensation left %+.2f%% of a %+.2f%% band\n", comp_band, plain_band);
    return 1;
  }

  // Black has to stay black: the dither offset must never push a zero channel
  // up a level, or every dark scene gains a haze.
  std::vector<uint16_t> black(W * H, rgb565(0, 0, 0));
  std::vector<uint16_t> black_out(W * H, 0xFFFF);
  const uint8_t strong[3] = {255, 255, 255};
  ssd1351_compensate_rows(black.data(), black_out.data(), W, H, strong);
  for(int i = 0; i < W * H; i++) {
    if(black_out[i] != 0) {
      std::fprintf(stderr, "black pixel %d came out as 0x%04X\n", i, black_out[i]);
      return 1;
    }
  }

  // A zero strength has to be a pass-through, so it can be used for an A/B.
  std::vector<uint16_t> passthru(W * H, 0);
  const uint8_t off[3] = {0, 0, 0};
  ssd1351_compensate_rows(frame.data(), passthru.data(), W, H, off);
  for(int i = 0; i < W * H; i++) {
    if(passthru[i] != frame[i]) {
      std::fprintf(stderr, "zero strength changed pixel %d: 0x%04X -> 0x%04X\n",
                   i, frame[i], passthru[i]);
      return 1;
    }
  }

  // Full white must not wrap round through the channel maxima.
  std::vector<uint16_t> white(W * H, rgb565(0x1F, 0x3F, 0x1F));
  std::vector<uint16_t> white_out(W * H, 0);
  ssd1351_compensate_rows(white.data(), white_out.data(), W, H, strong);
  for(int i = 0; i < W * H; i++) {
    if(white_out[i] != rgb565(0x1F, 0x3F, 0x1F)) {
      std::fprintf(stderr, "white pixel %d came out as 0x%04X\n", i, white_out[i]);
      return 1;
    }
  }

  // The remainder carried along each row has to start at a different phase per
  // row, or every row puts its rounded-up pixels at the same x and the
  // correction shows up as vertical stripes instead of a flat lift.
  std::vector<uint16_t> flat(W * H, rgb565(0, 0, 8));
  std::vector<uint16_t> flat_out(W * H);
  ssd1351_compensate_rows(flat.data(), flat_out.data(), W, H, kSsd1351RowCompensationDefault);

  double worst = 0;
  double frame_mean = 0;
  for(int i = 0; i < W * H; i++)
    frame_mean += flat_out[i] & 0x1F;
  frame_mean /= W * H;

  for(int x = 0; x < W; x++) {
    double col = 0;
    for(int y = 0; y < H; y++)
      col += flat_out[y * W + x] & 0x1F;
    col /= H;
    if(std::abs(col - frame_mean) > worst)
      worst = std::abs(col - frame_mean);
  }

  std::printf("worst column deviation on a flat field: %.3f levels\n", worst);
  if(worst > 0.15) {
    std::fprintf(stderr, "correction is landing in vertical stripes (%.3f levels)\n", worst);
    return 1;
  }

  std::printf("ssd1351 row compensation ok\n");
  return 0;
}
