#include <iterator>

#include "power.hpp"
#include "config.h"
#include "usb.hpp"

#ifdef BATTERY_ADC_PIN
#include "hardware/adc.h"

// voltage divider between the battery and the ADC pin
#ifndef BATTERY_DIVIDER
#define BATTERY_DIVIDER 1.0f
#endif

// drop between the battery and the divider (e.g. a diode in front of VSYS)
#ifndef BATTERY_OFFSET_MV
#define BATTERY_OFFSET_MV 0
#endif

static const uint32_t sample_interval_ms = 250;

static uint32_t last_sample_time = 0;
static uint32_t battery_mv = 0; // smoothed

// typical single cell LiPo discharge curve, mV to percent
static const struct {
  uint16_t mv;
  uint8_t percent;
} discharge_curve[] {
  {4200, 100}, {4110, 90}, {4020, 80}, {3950, 70}, {3870, 60},
  {3840, 50}, {3800, 40}, {3770, 30}, {3730, 20}, {3690, 10},
  {3610, 5}, {3270, 0},
};

static uint32_t read_battery_mv() {
  adc_select_input(BATTERY_ADC_PIN - ADC_BASE_PIN);

  uint32_t sum = 0;
  for(int i = 0; i < 8; i++)
    sum += adc_read();

  // 12 bit ADC, 3.3V reference
  return uint32_t(float(sum) / 8.0f * 3300.0f / 4095.0f * BATTERY_DIVIDER) + BATTERY_OFFSET_MV;
}
#endif

void init_power() {
#ifdef BATTERY_ADC_PIN
  adc_init();
  adc_gpio_init(BATTERY_ADC_PIN);
  battery_mv = read_battery_mv();
#endif
}

void update_power(uint32_t time) {
#ifdef BATTERY_ADC_PIN
  if(time - last_sample_time < sample_interval_ms)
    return;

  last_sample_time = time;
  battery_mv = (battery_mv * 7 + read_battery_mv()) / 8;
#endif
}

int get_battery_level() {
#ifdef BATTERY_ADC_PIN
  if(battery_mv >= discharge_curve[0].mv)
    return 100;

  for(unsigned i = 1; i < std::size(discharge_curve); i++) {
    auto &hi = discharge_curve[i - 1], &lo = discharge_curve[i];
    if(battery_mv >= lo.mv)
      return lo.percent + (battery_mv - lo.mv) * (hi.percent - lo.percent) / (hi.mv - lo.mv);
  }

  return 0;
#else
  return -1;
#endif
}

uint16_t get_battery_voltage() {
#ifdef BATTERY_ADC_PIN
  return battery_mv;
#else
  return 0;
#endif
}

bool is_usb_connected() {
  return usb_cdc_connected();
}
