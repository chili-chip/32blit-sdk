#include "ssd1351_init_seq.hpp"

#include <cstdio>

// Host-only check of the SSD1351 bring-up table. Not part of BlitHalPico.
//   g++ -std=c++17 -o /tmp/ssd1351_init_seq_test ssd1351_init_seq_test.cpp && /tmp/ssd1351_init_seq_test
int main() {
  bool saw_clock = false;
  bool saw_enhance = false;
  bool saw_precharge_level = false;
  bool saw_use_lut = false;
  bool saw_display_on = false;

  for(std::size_t i = 0; i < kSsd1351InitSeqCount; ++i) {
    const auto &c = kSsd1351InitSeq[i];
    if(c.cmd == kSsd1351CmdClockDiv) {
      saw_clock = true;
      if(c.nbytes != 1 || c.data[0] != 0xF0) {
        std::fprintf(stderr, "CLOCK_DIV (0xB3) is 0x%02X, expected 0xF0\n", c.data[0]);
        return 1;
      }
      if(c.data[0] != kSsd1351ClockDivDefault) {
        std::fprintf(stderr, "CLOCK_DIV mismatch vs kSsd1351ClockDivDefault\n");
        return 1;
      }
    }
    if(c.cmd == kSsd1351CmdEnhance) {
      saw_enhance = true;
      if(c.nbytes != 3 || c.data[0] != 0xA4 || c.data[1] != 0x00 || c.data[2] != 0x00) {
        std::fprintf(stderr, "DISPLAY_ENHANCE payload unexpected\n");
        return 1;
      }
    }
    if(c.cmd == kSsd1351CmdPrechargeLevel)
      saw_precharge_level = true;
    if(c.cmd == kSsd1351CmdUseLut)
      saw_use_lut = true;
    if(c.cmd == 0xAF)
      saw_display_on = true;
    if(c.cmd == 0xB3 && (c.data[0] & 0x0F) != 0) {
      std::fprintf(stderr, "CLOCK_DIV divider is /%u; want /1 (low nibble 0)\n",
                   (c.data[0] & 0x0F) + 1u);
      return 1;
    }
  }

  if(!saw_clock || !saw_enhance || !saw_precharge_level || !saw_use_lut) {
    std::fprintf(stderr, "init sequence missing required commands (clock=%d enhance=%d vpre=%d lut=%d)\n",
                 saw_clock, saw_enhance, saw_precharge_level, saw_use_lut);
    return 1;
  }
  if(saw_display_on) {
    std::fprintf(stderr, "DISPLAY_ON should be issued after SET_REMAP, not in the static table\n");
    return 1;
  }
  if(kSsd1351InitSeqCount < 12) {
    std::fprintf(stderr, "init sequence too short\n");
    return 1;
  }

  std::printf("ssd1351 init sequence ok: CLOCK_DIV=0x%02X, %zu commands\n",
              kSsd1351ClockDivDefault, kSsd1351InitSeqCount);
  return 0;
}
