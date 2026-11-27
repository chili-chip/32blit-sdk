#include "scale_map.hpp"

#include <cstdio>

// Host-only check of the nearest-neighbour scaler used by dbi_st7789_scaled.
// Not part of BlitHalPico.
//   g++ -std=c++17 -o /tmp/scale_map_test scale_map_test.cpp && /tmp/scale_map_test

static int failures = 0;

static void expect(bool ok, const char *what) {
  if(!ok) {
    std::fprintf(stderr, "FAIL: %s\n", what);
    failures++;
  }
}

static void check_fit(int src_w, int src_h, int want_w, int want_h) {
  auto s = fit_scaled(src_w, src_h, 320, 240);
  if(s.w != want_w || s.h != want_h) {
    std::fprintf(stderr, "FAIL: fit %dx%d -> %dx%d, expected %dx%d\n", src_w, src_h, s.w, s.h, want_w, want_h);
    failures++;
  }
}

// Every source pixel shows up, in order, between min and max times.
static void check_map(int out_len, int src_len, int min_rep, int max_rep) {
  uint16_t map[320];
  make_scale_map(map, out_len, src_len);

  expect(map[0] == 0, "map starts at source pixel 0");
  expect(map[out_len - 1] == src_len - 1, "map ends at the last source pixel");

  int run = 1;
  for(int i = 1; i < out_len; i++) {
    expect(map[i] == map[i - 1] || map[i] == map[i - 1] + 1, "map steps by 0 or 1");
    if(map[i] == map[i - 1]) {
      run++;
    } else {
      expect(run >= min_rep && run <= max_rep, "each source pixel repeats a fair number of times");
      run = 1;
    }
  }
  expect(run >= min_rep && run <= max_rep, "last source pixel repeats a fair number of times");
}

int main() {
  // 128x128 games (hires) and 64x64 (lores) fill the panel height, centred.
  check_fit(128, 128, 240, 240);
  check_fit(64, 64, 240, 240);
  // Landscape modes for future games fill the whole panel.
  check_fit(160, 120, 320, 240);
  check_fit(320, 240, 320, 240);

  check_map(240, 128, 1, 2); // 1.875x
  check_map(240, 64, 3, 4);  // 3.75x
  check_map(320, 160, 2, 2); // 2x
  check_map(240, 240, 1, 1); // 1x

  if(failures) return 1;
  std::puts("scale_map_test: ok");
  return 0;
}
