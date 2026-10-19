#include "flash_translation.hpp"

#include <cstdio>

// Host-only check of the flash translation sizing. Not part of BlitHalPico.
//   g++ -std=c++17 -o /tmp/flash_translation_test flash_translation_test.cpp && /tmp/flash_translation_test
static int failures = 0;

static void expect(uint32_t blit_size, uint32_t expected) {
  auto got = flash_translation_size(blit_size);
  if(got != expected) {
    std::fprintf(stderr, "flash_translation_size(%u) = %u, expected %u\n", blit_size, got, expected);
    failures++;
  }
}

int main() {
  // rounds up to whole 4k pages
  expect(1, 4096);
  expect(4096, 4096);
  expect(4097, 8192);
  expect(100 * 1024 + 10, 104 * 1024);

  // never more than the 4MB window
  expect(4 * 1024 * 1024, 4 * 1024 * 1024);
  expect(4 * 1024 * 1024 + 1, 4 * 1024 * 1024);
  expect(16 * 1024 * 1024, 4 * 1024 * 1024);

  // unknown size maps the whole window, like before
  expect(0, 4 * 1024 * 1024);

  return failures ? 1 : 0;
}
