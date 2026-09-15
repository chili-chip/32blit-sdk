#include "ssd1351_init_seq.hpp"

#include <cstdio>

#ifdef SSD1351_ROW_DRIVE_OVERRIDDEN
static constexpr bool check_recommended = false;
#else
static constexpr bool check_recommended = true;
#endif

// Host-only check of the SSD1351 bring-up table. Not part of BlitHalPico.
//   g++ -std=c++17 -o /tmp/ssd1351_init_seq_test ssd1351_init_seq_test.cpp && /tmp/ssd1351_init_seq_test
int main() {
  bool saw_clock = false;
  bool saw_enhance = false;
  bool saw_phase_12 = false;
  bool saw_precharge_2 = false;
  bool saw_precharge_level = false;
  bool saw_vsl = false;
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
    // Row cross-talk settings. A row that contains bright pixels tints the
    // rest of that row when the segment drivers are still charging pixel
    // capacitance during the current drive stage, or when the segment
    // reference is left to float, so keep these inside the datasheet ranges
    // rather than at the minimums that squeeze out a few percent of refresh.
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
      if(check_recommended && phase_2 < 8) {
        std::fprintf(stderr, "phase 2 is %u DCLK, below the power-on 8; short first "
                             "pre-charge causes row cross-talk\n", phase_2);
        return 1;
      }
    }
    if(c.cmd == kSsd1351CmdPrecharge2) {
      saw_precharge_2 = true;
      const unsigned phase_3 = c.data[0] & 0x0F;
      if(phase_3 < 1) {
        std::fprintf(stderr, "phase 3 (0xB6) is 0 DCLK, which is invalid\n");
        return 1;
      }
      if(check_recommended && phase_3 < 8) {
        std::fprintf(stderr, "phase 3 (0xB6) is %u DCLK, below the power-on 8; the pixel "
                             "should settle before the current drive stage\n", phase_3);
        return 1;
      }
    }
    if(c.cmd == kSsd1351CmdPrechargeLevel) {
      saw_precharge_level = true;
      if(c.data[0] > 0x1F) {
        std::fprintf(stderr, "pre-charge voltage (0xBB) 0x%02X is above 0x1F\n", c.data[0]);
        return 1;
      }
      if(check_recommended && c.data[0] > 0x17) {
        std::fprintf(stderr, "pre-charge voltage (0xBB) 0x%02X is above the power-on 0x17; "
                             "that parks dark pixels near turn-on\n", c.data[0]);
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
      if(check_recommended && (c.data[0] & 0x03) == 0x00) {
        std::fprintf(stderr, "SET_VSL selects external VSL; that needs the VSL-to-VSS "
                             "resistor and diode of datasheet figure 14-1\n");
        return 1;
      }
    }
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
  if(!saw_phase_12 || !saw_precharge_2 || !saw_vsl) {
    std::fprintf(stderr, "init sequence missing row drive commands (phase12=%d phase3=%d vsl=%d)\n",
                 saw_phase_12, saw_precharge_2, saw_vsl);
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
