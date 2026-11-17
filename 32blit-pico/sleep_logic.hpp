#pragma once
#include <cstdint>

// Decides when the console sleeps and wakes. No hardware here, so
// sleep_logic_test.cpp checks it on the host. sleep.cpp does the rest.
namespace sleep_logic {
  // hold Start this long to sleep
  constexpr uint32_t hold_to_sleep_ms = 2000;
  // and this long to wake
  constexpr uint32_t hold_to_wake_ms = 1000;
  // the battery must stay low this long, so a short dip doesn't count
  constexpr uint32_t low_battery_ms = 10000;

  // Stored in a watchdog scratch register across the reset into and out of sleep.
  // just asked to sleep: Start may still be held from the 2 s hold
  constexpr uint32_t phase_enter = 0x534c5045; // "SLPE"
  // woken by Start
  constexpr uint32_t phase_wake = 0x534c5057; // "SLPW"

  enum class Boot { Run, Sleep };

  // What to do at boot. start_held: Start stayed held for hold_to_wake_ms.
  constexpr Boot boot_action(uint32_t phase, bool start_held) {
    if(phase == phase_enter)
      return Boot::Sleep;
    if(phase == phase_wake)
      return start_held ? Boot::Run : Boot::Sleep;
    return Boot::Run;
  }

  // Hold Start to sleep, and sleep when the battery runs low.
  class Trigger {
  public:
    // Call once a frame. Returns true when it's time to sleep.
    // battery_mv 0 (no reading) or low_battery_mv 0 turns the battery check off.
    bool update(uint32_t now_ms, bool start_held, uint16_t battery_mv, uint16_t low_battery_mv) {
      // Start held since boot (the wake hold) only counts once it's let go
      if(!start_held)
        armed = true;

      if(start_held && armed) {
        if(!holding)
          hold_start = now_ms;
        holding = true;
      } else
        holding = false;

      bool low = battery_mv && low_battery_mv && battery_mv < low_battery_mv;
      if(low && !battery_low)
        low_start = now_ms;
      battery_low = low;

      return (holding && now_ms - hold_start >= hold_to_sleep_ms)
          || (battery_low && now_ms - low_start >= low_battery_ms);
    }

    // 0 to 1 while Start is held, for a "keep holding" hint
    float hold_progress(uint32_t now_ms) const {
      if(!holding)
        return 0.0f;
      float progress = float(now_ms - hold_start) / hold_to_sleep_ms;
      return progress > 1.0f ? 1.0f : progress;
    }

  private:
    bool armed = false;
    bool holding = false;
    uint32_t hold_start = 0;
    bool battery_low = false;
    uint32_t low_start = 0;
  };
}
