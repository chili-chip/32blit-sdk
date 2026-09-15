#include "ssd1351_init_seq.hpp"

#include <cstdio>

// Host-only check of the SSD1351 bring-up table. Not part of BlitHalPico.
//   g++ -std=c++17 -o /tmp/ssd1351_init_seq_test ssd1351_init_seq_test.cpp && /tmp/ssd1351_init_seq_test
//
// Checks structure and datasheet-legal ranges only. It deliberately does not
// pin the analog values to one setting: which of them a panel wants is a
// bench question, and every one of them is overridable.
int main() {
  bool saw_clock = false;
  bool saw_enhance = false;
  bool saw_phase_12 = false;
  bool saw_precharge_2 = false;
  bool saw_precharge_level = false;
  bool saw_vsl = false;
  bool saw_contrast_abc = false;
  bool saw_contrast_master = false;
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
      if((c.data[0] & 0x0F) != 0) {
        std::fprintf(stderr, "CLOCK_DIV divider is /%u; want /1 (low nibble 0)\n",
                     (c.data[0] & 0x0F) + 1u);
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
    if(c.cmd == kSsd1351CmdPhase12) {
      saw_phase_12 = true;
      const unsigned phase_1 = c.data[0] & 0x0F;
      const unsigned phase_2 = c.data[0] >> 4;
      if(phase_1 < 2) {
        std::fprintf(stderr, "phase 1 (0xB1 A[3:0]) is %u; 0 and 1 are invalid\n", phase_1);
        return 1;
      }
      if(phase_2 < 3) {
        std::fprintf(stderr, "phase 2 (0xB1 A[7:4]) is %u; 0 to 2 are invalid\n", phase_2);
        return 1;
      }
    }
    if(c.cmd == kSsd1351CmdPrecharge2) {
      saw_precharge_2 = true;
      if((c.data[0] & 0x0F) < 1) {
        std::fprintf(stderr, "phase 3 (0xB6) is 0 DCLK, which is invalid\n");
        return 1;
      }
    }
    if(c.cmd == kSsd1351CmdPrechargeLevel) {
      saw_precharge_level = true;
      if(c.data[0] > 0x1F) {
        std::fprintf(stderr, "pre-charge voltage (0xBB) 0x%02X is above 0x1F\n", c.data[0]);
        return 1;
      }
    }
    if(c.cmd == kSsd1351CmdVsl) {
      saw_vsl = true;
      if(c.nbytes != 3 || (c.data[0] & 0xFC) != 0xA0 || c.data[1] != 0xB5 || c.data[2] != 0x55) {
        std::fprintf(stderr, "SET_VSL payload unexpected (A[7:2] is fixed at 101000b)\n");
        return 1;
      }
      if((c.data[0] & 0x03) != 0x00 && (c.data[0] & 0x03) != 0x02) {
        std::fprintf(stderr, "SET_VSL A[1:0] is %u; only 00b and 10b are defined\n", c.data[0] & 0x03);
        return 1;
      }
    }
    if(c.cmd == kSsd1351CmdContrastAbc) {
      saw_contrast_abc = true;
      if(c.nbytes != 3) {
        std::fprintf(stderr, "CONTRAST_ABC (0xC1) takes three bytes, got %u\n", c.nbytes);
        return 1;
      }
    }
    if(c.cmd == kSsd1351CmdContrastMaster) {
      saw_contrast_master = true;
      if(c.data[0] > 0x0F) {
        std::fprintf(stderr, "master contrast (0xC7) 0x%02X is above 0x0F\n", c.data[0]);
        return 1;
      }
    }
    if(c.cmd == kSsd1351CmdUseLut)
      saw_use_lut = true;
    if(c.cmd == 0xAF)
      saw_display_on = true;
  }

  if(!saw_clock || !saw_enhance || !saw_precharge_level || !saw_use_lut) {
    std::fprintf(stderr, "init sequence missing required commands (clock=%d enhance=%d vpre=%d lut=%d)\n",
                 saw_clock, saw_enhance, saw_precharge_level, saw_use_lut);
    return 1;
  }
  if(!saw_phase_12 || !saw_precharge_2 || !saw_vsl) {
    std::fprintf(stderr, "init sequence missing segment waveform commands (phase12=%d phase3=%d vsl=%d)\n",
                 saw_phase_12, saw_precharge_2, saw_vsl);
    return 1;
  }
  if(!saw_contrast_abc || !saw_contrast_master) {
    std::fprintf(stderr, "init sequence missing drive current commands (abc=%d master=%d)\n",
                 saw_contrast_abc, saw_contrast_master);
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

  std::printf("ssd1351 init sequence ok: CLOCK_DIV=0x%02X, master contrast 0x%02X, %zu commands\n",
              kSsd1351ClockDivDefault, static_cast<unsigned>(SSD1351_CONTRAST_MASTER),
              kSsd1351InitSeqCount);
  return 0;
}
