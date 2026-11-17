#include "sleep_logic.hpp"

#include <cstdio>

// Host-only check of when the console sleeps and wakes. Not part of BlitHalPico.
//   g++ -std=c++17 -o /tmp/sleep_logic_test sleep_logic_test.cpp && /tmp/sleep_logic_test
using namespace sleep_logic;

static int failures = 0;

static void check(bool ok, const char *what) {
  if(!ok) {
    std::fprintf(stderr, "FAIL: %s\n", what);
    failures++;
  }
}

static constexpr uint16_t full_mv = 4100, low_mv = 3400, limit_mv = 3450;

int main() {
  // boot: a normal boot runs
  check(boot_action(0, false) == Boot::Run, "normal boot runs");
  check(boot_action(0x12345678, true) == Boot::Run, "unknown scratch value runs");
  // just asked to sleep: always sleep, even with Start still held from the 2 s hold
  check(boot_action(phase_enter, true) == Boot::Sleep, "enter sleeps with Start held");
  check(boot_action(phase_enter, false) == Boot::Sleep, "enter sleeps");
  // woken: run only if Start stayed held, else back to sleep
  check(boot_action(phase_wake, true) == Boot::Run, "wake with Start held runs");
  check(boot_action(phase_wake, false) == Boot::Sleep, "short press goes back to sleep");

  // hold Start for 2 s to sleep
  {
    Trigger t;
    check(!t.update(0, false, full_mv, limit_mv), "idle");
    check(!t.update(100, true, full_mv, limit_mv), "hold starts");
    check(t.hold_progress(1100) > 0.49f && t.hold_progress(1100) < 0.51f, "half way after 1 s");
    check(!t.update(2099, true, full_mv, limit_mv), "not yet at 1.999 s");
    check(t.update(2100, true, full_mv, limit_mv), "sleeps at 2 s");
  }

  // letting go cancels
  {
    Trigger t;
    t.update(0, false, full_mv, limit_mv);
    t.update(100, true, full_mv, limit_mv);
    t.update(1500, false, full_mv, limit_mv);
    check(t.hold_progress(1500) == 0.0f, "no progress after release");
    check(!t.update(1600, true, full_mv, limit_mv), "new hold starts over");
    check(!t.update(3000, true, full_mv, limit_mv), "1.4 s into the new hold");
    check(t.update(3600, true, full_mv, limit_mv), "2 s into the new hold");
  }

  // Start held from boot (the wake hold) doesn't count until it's let go once
  {
    Trigger t;
    check(!t.update(0, true, full_mv, limit_mv), "held at boot");
    check(!t.update(5000, true, full_mv, limit_mv), "still held from the wake, no sleep");
    check(t.hold_progress(5000) == 0.0f, "no progress while held from boot");
    t.update(5100, false, full_mv, limit_mv);
    t.update(5200, true, full_mv, limit_mv);
    check(t.update(7200, true, full_mv, limit_mv), "a new hold after release sleeps");
  }

  // low battery: sleeps after 10 s below the limit
  {
    Trigger t;
    check(!t.update(0, false, low_mv, limit_mv), "just went low");
    check(!t.update(9999, false, low_mv, limit_mv), "low for under 10 s");
    check(t.update(10000, false, low_mv, limit_mv), "low for 10 s sleeps");
  }

  // a dip that recovers (a sound or the motor) does not
  {
    Trigger t;
    t.update(0, false, low_mv, limit_mv);
    t.update(5000, false, full_mv, limit_mv);
    check(!t.update(12000, false, low_mv, limit_mv), "low again, timer restarted");
    check(!t.update(21999, false, low_mv, limit_mv), "under 10 s since it went low again");
    check(t.update(22000, false, low_mv, limit_mv), "10 s since it went low again");
  }

  // no battery reading (0 mV) or no limit set never sleeps
  {
    Trigger t;
    t.update(0, false, 0, limit_mv);
    check(!t.update(60000, false, 0, limit_mv), "no reading");
    Trigger u;
    u.update(0, false, low_mv, 0);
    check(!u.update(60000, false, low_mv, 0), "no limit");
  }

  if(failures)
    std::fprintf(stderr, "%i failure(s)\n", failures);
  else
    std::printf("sleep_logic: all checks passed\n");

  return failures ? 1 : 0;
}
