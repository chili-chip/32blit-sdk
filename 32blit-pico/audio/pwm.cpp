// Hardware PWM audio DAC (not the PicoSystem beep driver, not pico-extras PIO PWM).
//
// Core 1 writes PWM duty at 22050 Hz from blit::get_audio_frame(). The whole
// hot path (this function + the mixer) must run from RAM: a flash-resident
// mix loop starves core 0 of XIP and freezes the console when a game loads.

#include "audio.hpp"
#include "config.h"

#include "hardware/gpio.h"
#include "hardware/pwm.h"
#include "pico/platform.h"
#include "pico/time.h"

#include "audio/audio.hpp"

#ifndef PICO_AUDIO_PWM_MONO_PIN
#error "PICO_AUDIO_PWM_MONO_PIN must be defined for the PWM audio driver"
#endif

#define AUDIO_SAMPLE_FREQ 22050
#define PWM_WRAP 255u

static uint audio_pin;
static uint32_t sample_us;
static uint32_t next_sample_us;

void init_audio() {
  audio_pin = PICO_AUDIO_PWM_MONO_PIN;
  const uint slice = pwm_gpio_to_slice_num(audio_pin);

#ifdef AUDIO_ENABLE_PIN
  gpio_init(AUDIO_ENABLE_PIN);
  gpio_set_dir(AUDIO_ENABLE_PIN, GPIO_OUT);
#ifdef AUDIO_ENABLE_ACTIVE_LOW
  gpio_put(AUDIO_ENABLE_PIN, 0);
#else
  gpio_put(AUDIO_ENABLE_PIN, 1);
#endif
#endif

  gpio_set_function(audio_pin, GPIO_FUNC_PWM);
  gpio_set_drive_strength(audio_pin, GPIO_DRIVE_STRENGTH_12MA);
  gpio_set_slew_rate(audio_pin, GPIO_SLEW_RATE_FAST);

  pwm_config cfg = pwm_get_default_config();
  pwm_config_set_clkdiv(&cfg, 1.f);
  pwm_config_set_wrap(&cfg, PWM_WRAP);
  pwm_init(slice, &cfg, true);
  pwm_set_gpio_level(audio_pin, (PWM_WRAP + 1) / 2);

  sample_us = 1000000u / AUDIO_SAMPLE_FREQ;
  next_sample_us = time_us_32();
}

void __not_in_flash_func(update_audio)(uint32_t time) {
  (void)time;

  const uint32_t now = time_us_32();
  const int32_t delta = (int32_t)(now - next_sample_us);
  if(delta < 0)
    return;

  // After flash lockout / a long stall, skip the backlog.
  if(delta > 2000)
    next_sample_us = now;

  const uint16_t sample = blit::get_audio_frame();
  pwm_set_gpio_level(audio_pin, static_cast<uint16_t>(
    (static_cast<uint32_t>(sample) * PWM_WRAP) / 0xffffu));
  next_sample_us += sample_us;
}
