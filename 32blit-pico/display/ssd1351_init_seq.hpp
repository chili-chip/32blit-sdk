#pragma once

#include <cstddef>
#include <cstdint>

/// SSD1351 command 0xB3: Front Clock Divider / Oscillator Frequency.
///
/// Bits 7:4 = oscillator frequency (higher is faster PWM/multiplex).
/// Bits 3:0 = DCLK divider (value + 1).
///
/// 0xF1 (max osc, divide-by-2) halves panel refresh versus 0xF0 and is the
/// main source of rolling bars on phone cameras and flicker when the
/// handheld is moved. Override at compile time if a panel cannot tolerate /1:
///   -DSSD1351_CLOCK_DIV=0xF1
#ifndef SSD1351_CLOCK_DIV
#define SSD1351_CLOCK_DIV 0xF0
#endif

/// Command 0xC7 power-on master contrast (0–15). Brightness sliders remap
/// this at runtime via ssd1351_set_master_contrast(); this is only the
/// value programmed during init.
#ifndef SSD1351_CONTRAST_MASTER
#define SSD1351_CONTRAST_MASTER 0x0F
#endif

// ---------------------------------------------------------------------------
// Row cross-talk
// ---------------------------------------------------------------------------
// The SSD1351 drives a passive matrix: one COM (row) is selected at a time and
// every lit segment in that row sinks its current through that one COM
// electrode. Anything that lets the row's shared references move with the
// row's total current shows up as a tinted horizontal band across the
// otherwise-solid background of every row that happens to contain bright
// pixels. The settings below are the ones that decide how much of that the
// driver generates; the defaults are the datasheet power-on values, which is
// what a panel is characterised against.

// A board that overrides any of them opts out of the recommended-value half
// of ssd1351_init_seq_test.cpp; the datasheet range checks still apply.
#if defined(SSD1351_VSL_SELECT) || defined(SSD1351_PHASE_12) || \
    defined(SSD1351_PRECHARGE_2) || defined(SSD1351_PRECHARGE_LEVEL)
#define SSD1351_ROW_DRIVE_OVERRIDDEN 1
#endif

/// Command 0xB4 byte A: segment low voltage (VSL) source. A[7:2] is fixed at
/// 101000b, A[1:0] selects:
///   0xA0 - external VSL (power-on default)
///   0xA2 - internal VSL, VSL pin left open
/// External VSL only works if the module wires a resistor and diode from the
/// VSL pin down to VSS (datasheet figure 14-1). The 0xB4 note spells out that
/// the circuit is required "in order to avoid distortion in display pattern":
/// with the pin open there is nothing holding the segment reference, so it
/// follows the row's drive current and tints the whole row. Pass 0xA0 for a
/// module that carries the network.
#ifndef SSD1351_VSL_SELECT
#define SSD1351_VSL_SELECT 0xA2
#endif

/// Command 0xB1: phase 1 (reset) length in A[3:0], phase 2 (first pre-charge)
/// length in A[7:4]. A[3:0] counts 2 DCLK per step from 2 = 5 DCLK to
/// 15 = 31 DCLK; A[7:4] counts 1 DCLK per step from 3 = 3 DCLK to
/// 15 = 15 DCLK. 0x82 is the power-on value: phase 1 = 5 DCLK,
/// phase 2 = 8 DCLK.
#ifndef SSD1351_PHASE_12
#define SSD1351_PHASE_12 0x82
#endif

/// Command 0xB6: phase 3 (second pre-charge) length, 1 to 15 DCLK, power-on 8.
///
/// Phase 3 walks the pixel to its target drive voltage before the constant
/// current stage takes over. Running phases 2 and 3 at their minimum
/// (0x32 / 0x01) leaves the segment current sources still charging pixel
/// capacitance once the drive stage starts, so how bright a pixel ends up
/// depends on how loaded the rest of its row is. The refresh rate it buys back
/// is small: a row period is phase 1 + phase 2 + drive DCLKs, so the power-on
/// periods cost about 8% against the minimums, which the 0xF0 divider above
/// already more than covers.
#ifndef SSD1351_PRECHARGE_2
#define SSD1351_PRECHARGE_2 0x08
#endif

/// Command 0xBB: phase 2 pre-charge voltage, 0x00 = 0.20 x VCC up to
/// 0x1F = 0.60 x VCC, power-on 0x17. Every segment in the selected row gets
/// pre-charged to this level, dark ones included, so the maximum parks dark
/// pixels just below their turn-on point where a small shift in the row's
/// references is enough to light them.
#ifndef SSD1351_PRECHARGE_LEVEL
#define SSD1351_PRECHARGE_LEVEL 0x17
#endif

/// Command 0xC1 per-colour drive current. Lowering these lowers the current a
/// bright row pulls through its COM electrode, and so the IR drop along it.
/// This is the knob left if a panel still bands after the above; it costs
/// brightness, so it is not turned down by default.
#ifndef SSD1351_CONTRAST_A
#define SSD1351_CONTRAST_A 0xC8
#endif
#ifndef SSD1351_CONTRAST_B
#define SSD1351_CONTRAST_B 0x80
#endif
#ifndef SSD1351_CONTRAST_C
#define SSD1351_CONTRAST_C 0xC8
#endif

struct Ssd1351Cmd {
  uint8_t cmd;
  uint8_t nbytes;
  uint8_t data[4];
};

/// Panel bring-up before SET_REMAP / START_LINE / window.
/// DISPLAY_ON is issued after those so the first GRAM fill is not scanned.
inline constexpr Ssd1351Cmd kSsd1351InitSeq[] = {
  {0xFD, 1, {0x12}},                          // COMMAND_LOCK: unlock OLED driver
  {0xFD, 1, {0xB1}},                          // COMMAND_LOCK: unlock command set
  {0xAE, 0, {}},                              // DISPLAY_OFF
  {0xB2, 3, {0xA4, 0x00, 0x00}},              // DISPLAY_ENHANCE (A)
  {0xB3, 1, {static_cast<uint8_t>(SSD1351_CLOCK_DIV)}}, // CLOCK_DIV
  {0xCA, 1, {0x7F}},                          // MUX_RATIO = 127 (128 rows)
  {0xA2, 1, {0x00}},                          // DISPLAY_OFFSET
  {0xB5, 1, {0x00}},                          // SET_GPIO disabled
  {0xAB, 1, {0x01}},                          // FUNCTION_SELECT: internal VDD
  {0xB1, 1, {static_cast<uint8_t>(SSD1351_PHASE_12)}},        // PRECHARGE phase 1/2
  {0xBB, 1, {static_cast<uint8_t>(SSD1351_PRECHARGE_LEVEL)}}, // PRECHARGE_LEVEL
  {0xBE, 1, {0x05}},                          // VCOMH
  {0xA6, 0, {}},                              // NORMAL_DISPLAY
  {0xC1, 3, {static_cast<uint8_t>(SSD1351_CONTRAST_A),
             static_cast<uint8_t>(SSD1351_CONTRAST_B),
             static_cast<uint8_t>(SSD1351_CONTRAST_C)}},      // CONTRAST_ABC
  {0xC7, 1, {static_cast<uint8_t>(SSD1351_CONTRAST_MASTER)}}, // CONTRAST_MASTER
  {0xB4, 3, {static_cast<uint8_t>(SSD1351_VSL_SELECT), 0xB5, 0x55}}, // SET_VSL
  {0xB9, 0, {}},                              // USE_LUT: linear grayscale
  {0xB6, 1, {static_cast<uint8_t>(SSD1351_PRECHARGE_2)}},     // PRECHARGE_2
};

inline constexpr std::size_t kSsd1351InitSeqCount =
  sizeof(kSsd1351InitSeq) / sizeof(kSsd1351InitSeq[0]);

inline constexpr uint8_t kSsd1351ClockDivDefault = static_cast<uint8_t>(SSD1351_CLOCK_DIV);
inline constexpr uint8_t kSsd1351CmdPhase12 = 0xB1;
inline constexpr uint8_t kSsd1351CmdClockDiv = 0xB3;
inline constexpr uint8_t kSsd1351CmdEnhance = 0xB2;
inline constexpr uint8_t kSsd1351CmdVsl = 0xB4;
inline constexpr uint8_t kSsd1351CmdPrecharge2 = 0xB6;
inline constexpr uint8_t kSsd1351CmdPrechargeLevel = 0xBB;
inline constexpr uint8_t kSsd1351CmdUseLut = 0xB9;
