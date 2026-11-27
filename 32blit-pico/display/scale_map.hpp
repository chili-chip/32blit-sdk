#pragma once
#include <cstdint>

// Nearest-neighbour scaling helpers for dbi_st7789_scaled.cpp.
// Plain C++ so the host test (scale_map_test.cpp) can check them.

struct ScaledSize {
  int w, h;
};

// Largest size with the source's aspect ratio that fits max_w x max_h.
inline ScaledSize fit_scaled(int src_w, int src_h, int max_w, int max_h) {
  int w = src_w * max_h / src_h;
  if(w <= max_w)
    return {w, max_h};
  return {max_w, src_h * max_w / src_w};
}

// Output pixel i shows source pixel map[i].
inline void make_scale_map(uint16_t *map, int out_len, int src_len) {
  for(int i = 0; i < out_len; i++)
    map[i] = i * src_len / out_len;
}
