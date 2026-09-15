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
  {0xB1, 1, {0x32}},                          // PRECHARGE phase 1/2
  {0xBB, 1, {0x1F}},                          // PRECHARGE_LEVEL
  {0xBE, 1, {0x05}},                          // VCOMH
  {0xA6, 0, {}},                              // NORMAL_DISPLAY
  {0xC1, 3, {0xC8, 0x80, 0xC8}},              // CONTRAST_ABC
  {0xC7, 1, {static_cast<uint8_t>(SSD1351_CONTRAST_MASTER)}}, // CONTRAST_MASTER
  {0xB4, 3, {0xA0, 0xB5, 0x55}},              // SET_VSL
  {0xB9, 0, {}},                              // USE_LUT: linear grayscale
  {0xB6, 1, {0x01}},                          // PRECHARGE_2 (keep short → higher FPS)
};

inline constexpr std::size_t kSsd1351InitSeqCount =
  sizeof(kSsd1351InitSeq) / sizeof(kSsd1351InitSeq[0]);

inline constexpr uint8_t kSsd1351ClockDivDefault = static_cast<uint8_t>(SSD1351_CLOCK_DIV);
inline constexpr uint8_t kSsd1351CmdClockDiv = 0xB3;
inline constexpr uint8_t kSsd1351CmdEnhance = 0xB2;
inline constexpr uint8_t kSsd1351CmdPrechargeLevel = 0xBB;
inline constexpr uint8_t kSsd1351CmdUseLut = 0xB9;
