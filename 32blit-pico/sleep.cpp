// Sleep mode for the console, which has no power switch (32blit-sdk #97).
//
// Going to sleep resets the chip, so nothing from the running game or the
// USB stack is left over:
//
//   sleep_enter()  parts off, scratch = phase_enter, watchdog reset
//   boot           wait for Start to be let go, scratch = phase_wake,
//                  DORMANT until Start goes low, watchdog reset
//   boot           Start held for 1 s: start as normal; else back to DORMANT
//
// While DORMANT every clock is stopped. The display is held in reset (on the
// TFT version that also turns off the backlight), the amplifier and radio are
// off. The utility module's parts are in power-down from their own reset, as
// no driver turns them on yet.
#include "sleep.hpp"
#include "config.h"

#ifdef SLEEP_BUTTON_PIN

#include "hardware/clocks.h"
#include "hardware/gpio.h"
#include "hardware/pll.h"
#include "hardware/structs/rosc.h"
#include "hardware/watchdog.h"
#include "hardware/xosc.h"
#include "pico/time.h"

#include "display.hpp"
#include "power.hpp"
#include "screen_sleep.hpp"
#include "settings.hpp"
#include "sleep_logic.hpp"
#include "usb.hpp"

using namespace sleep_logic;

#ifndef SLEEP_BATTERY_MV
#define SLEEP_BATTERY_MV 0
#endif

// watchdog scratch 0-3 are ours, 4-7 belong to watchdog_reboot and the boot ROM
static constexpr int phase_scratch = 0;

static Trigger trigger;
static bool woke = false;

static bool start_down() {
  return !gpio_get(SLEEP_BUTTON_PIN);
}

static void hold_low(int pin) {
  gpio_init(pin);
  gpio_put(pin, 0);
  gpio_set_dir(pin, GPIO_OUT);
}

static void hold_parts_off() {
  // display in reset, which also turns off the TFT's backlight (vgc-zero-hardware #41)
  hold_low(LCD_RESET_PIN);
#ifdef AUDIO_ENABLE_PIN
  hold_low(AUDIO_ENABLE_PIN);
#endif
#ifdef RADIO_POWER_PIN
  hold_low(RADIO_POWER_PIN);
#endif
}

// Stop every clock until Start goes low, then reset.
[[noreturn]] static void dormant_until_start() {
  // run from the crystal with the PLLs and the ring oscillator off, so the crystal can stop
  clock_configure_undivided(clk_ref, CLOCKS_CLK_REF_CTRL_SRC_VALUE_XOSC_CLKSRC, 0, XOSC_HZ);
  clock_configure_undivided(clk_sys, CLOCKS_CLK_SYS_CTRL_SRC_VALUE_CLK_REF, 0, XOSC_HZ);
  clock_stop(clk_adc);
  clock_stop(clk_usb);
#if !PICO_RP2040
  clock_stop(clk_hstx);
#endif
  clock_configure_undivided(clk_peri, 0, CLOCKS_CLK_PERI_CTRL_AUXSRC_VALUE_CLK_SYS, XOSC_HZ);
  pll_deinit(pll_sys);
  pll_deinit(pll_usb);
  hw_write_masked(&rosc_hw->ctrl, ROSC_CTRL_ENABLE_VALUE_DISABLE << ROSC_CTRL_ENABLE_LSB, ROSC_CTRL_ENABLE_BITS);

  // level, not edge: a press between the release check and here still wakes
  gpio_set_dormant_irq_enabled(SLEEP_BUTTON_PIN, IO_BANK0_DORMANT_WAKE_INTE0_GPIO0_LEVEL_LOW_BITS, true);
  xosc_dormant();

  // Start is down: the next boot checks it's held for long enough
  watchdog_reboot(0, 0, 0);
  while(true)
    tight_loop_contents();
}

// true if Start stays down for ms
static bool start_held_for(uint32_t ms) {
  auto start = time_us_32();
  while(time_us_32() - start < ms * 1000) {
    if(!start_down())
      return false;
  }
  return true;
}

void sleep_check_at_boot() {
  uint32_t phase = watchdog_hw->scratch[phase_scratch];
  watchdog_hw->scratch[phase_scratch] = 0;

  if(phase != phase_enter && phase != phase_wake)
    return;

  gpio_init(SLEEP_BUTTON_PIN);
  gpio_pull_up(SLEEP_BUTTON_PIN);
  sleep_us(100); // let the pull-up settle

  bool held = phase == phase_wake && start_held_for(hold_to_wake_ms);
  if(boot_action(phase, held) == Boot::Run) {
    woke = true;
    return;
  }

  hold_parts_off();

  // wait until Start has been up for 50 ms (it's still down after the hold to sleep)
  auto up_since = time_us_32();
  while(time_us_32() - up_since < 50000) {
    if(start_down())
      up_since = time_us_32();
  }

  watchdog_hw->scratch[phase_scratch] = phase_wake;
  dormant_until_start();
}

bool sleep_woke() {
  return woke;
}

bool sleep_wanted(uint32_t time) {
  if(trigger.update(time, start_down(), get_battery_voltage(), SLEEP_BATTERY_MV))
    return true;

  // the idle timeout from the settings, unless a computer has the drive open
  uint32_t timeout_ms = uint32_t(get_current_settings().sleep_timeout) * 1000;
  return timeout_ms && screen_idle_ms(time) >= timeout_ms && !usb_storage_in_use();
}

void sleep_enter() {
  // panel off cleanly before its reset line drops
  set_display_power(false);
  hold_parts_off();

  watchdog_hw->scratch[phase_scratch] = phase_enter;
  watchdog_reboot(0, 0, 0);
  while(true)
    tight_loop_contents();
}

#else

void sleep_check_at_boot() {}
bool sleep_woke() { return false; }
bool sleep_wanted(uint32_t) { return false; }
void sleep_enter() {
  while(true) {}
}

#endif
