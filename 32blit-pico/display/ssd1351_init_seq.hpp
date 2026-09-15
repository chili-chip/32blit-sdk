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

// ---------------------------------------------------------------------------
// Row loading
// ---------------------------------------------------------------------------
// The SSD1351 drives a passive matrix: one COM (row) is selected at a time and
// every lit segment in that row sinks its current through that one COM
// electrode. A row with a lot of lit pixels therefore pulls a much larger
// current spike than a row of dark background, and on this panel that shows up
// as a horizontal band: every row containing bright pixels loses about 8-9% of
// its background luminance, uniformly across the full width of the row, and
// shifts towards red because blue and green lose more than red.
//
// Measured off a photo of the panel, on three background strips 10, 21 and 32
// columns in from the left edge, against rows whose total lit content came
// from a UI box several hundred columns away. All three dip by the same amount
// on the same rows.
//
// That signature is supply droop, not a timing or reference setting. The
// segment drivers are current sources, so a sagging VCC changes nothing until
// the sag eats their compliance headroom; then they drop out of regulation and
// the colour with the highest forward voltage goes first, which is blue. That
// is why the pre-charge and VSL settings below make no difference to it, and
// why the drive current does: less current means less droop, and less droop
// means the drivers stay in regulation.
//
// The real fix is on the board - VCC and VCOMH decoupling at the panel, per
// the datasheet application circuit. The lever here is the drive current.

/// Command 0xC7 master contrast (0-15), the coarse scaler on segment drive
/// current. Brightness sliders remap this at runtime via
/// ssd1351_set_master_contrast(); this is only the value programmed at init.
///
/// Note that turning this down does not touch the banding, which is why it is
/// left at maximum. It scales every pixel's drive current by the same factor,
/// so the sag and the background's own brightness come down together and the
/// percentage dip lands where it started. That is what the compensation pass
/// in ssd1351_row_compensation.hpp is for.
#ifndef SSD1351_CONTRAST_MASTER
#define SSD1351_CONTRAST_MASTER 0x0F
#endif

/// Command 0xC1 per-colour drive current. The fine control under the master
/// contrast above, and unlike it a way to change one colour relative to the
/// others: blue has the highest forward voltage and so the least headroom when
/// a row sags. ssd1351_set_contrast_abc() sweeps these at runtime.
#ifndef SSD1351_CONTRAST_A
#define SSD1351_CONTRAST_A 0xC8
#endif
#ifndef SSD1351_CONTRAST_B
#define SSD1351_CONTRAST_B 0x80
#endif
#ifndef SSD1351_CONTRAST_C
#define SSD1351_CONTRAST_C 0xC8
#endif

// ---------------------------------------------------------------------------
// Segment waveform
// ---------------------------------------------------------------------------
// These were tried against the banding above and made no difference to it, so
// they stay on the values the panel has been running. They are broken out
// because they are the settings worth sweeping for any *other* segment
// artefact (smearing, ghosting, dark-pixel glow), and ssd1351_set_row_drive()
// writes all four at runtime.

/// Command 0xB4 byte A: segment low voltage (VSL) source. A[7:2] is fixed at
/// 101000b, A[1:0] selects:
///   0xA0 - external VSL (power-on default)
///   0xA2 - internal VSL, VSL pin left open
/// External VSL wants a resistor and diode from the VSL pin down to VSS
/// (datasheet figure 14-1); the 0xB4 note says that circuit is needed "in
/// order to avoid distortion in display pattern". Whether this board carries
/// it is unconfirmed, but switching to internal VSL changed nothing visible,
/// so the reference inits' 0xA0 stands.
#ifndef SSD1351_VSL_SELECT
#define SSD1351_VSL_SELECT 0xA0
#endif

/// Command 0xB1: phase 1 (reset) length in A[3:0], phase 2 (first pre-charge)
/// length in A[7:4]. A[3:0] counts 2 DCLK per step from 2 = 5 DCLK to
/// 15 = 31 DCLK; A[7:4] counts 1 DCLK per step from 3 = 3 DCLK to
/// 15 = 15 DCLK. 0x32 is phase 1 = 5 DCLK (the power-on value) and phase 2 =
/// 3 DCLK (the documented minimum); power-on for phase 2 is 8.
#ifndef SSD1351_PHASE_12
#define SSD1351_PHASE_12 0x32
#endif

/// Command 0xB6: phase 3 (second pre-charge) length, 1 to 15 DCLK, power-on 8.
/// A row period is phase 1 + phase 2 + drive DCLKs, so the minimum here and in
/// phase 2 above is worth about 8% of refresh rate over the power-on values.
#ifndef SSD1351_PRECHARGE_2
#define SSD1351_PRECHARGE_2 0x01
#endif

/// Command 0xBB: phase 2 pre-charge voltage, 0x00 = 0.20 x VCC up to
/// 0x1F = 0.60 x VCC, power-on 0x17. Every segment in the selected row is
/// pre-charged to this level, dark ones included, so the maximum is what to
/// back off if dark pixels glow.
#ifndef SSD1351_PRECHARGE_LEVEL
#define SSD1351_PRECHARGE_LEVEL 0x1F
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
inline constexpr uint8_t kSsd1351CmdContrastAbc = 0xC1;
inline constexpr uint8_t kSsd1351CmdContrastMaster = 0xC7;
