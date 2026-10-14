#include "screen_sleep.hpp"
#include "display.hpp"
#include "installer.hpp"
#include "settings.hpp"

#include "engine/api_private.hpp"

// the screen dims this long before it blanks (or halfway, for short timeouts)
static constexpr uint32_t dim_time_ms = 10000;
// brightness while dimmed, if the setting is higher
static constexpr uint8_t dim_brightness = 2;

enum class ScreenState {
  On,
  Dimmed,
  Off,
};

static ScreenState state = ScreenState::On;
static uint32_t last_activity = 0;
static uint32_t last_buttons = 0;
static blit::Vec2 last_joystick;
// hide buttons from the game until they're all released after waking
static bool swallow_buttons = false;

static bool joystick_moved() {
  auto &joystick = blit::api_data.joystick;
  return joystick.x != last_joystick.x || joystick.y != last_joystick.y;
}

static void set_state(ScreenState new_state) {
  if(new_state == state)
    return;

  auto old_state = state;
  state = new_state;

  if(old_state == ScreenState::Off)
    set_display_power(true);

  apply_screen_brightness();

  if(new_state == ScreenState::Off)
    set_display_power(false);
}

void apply_screen_brightness() {
  auto brightness = get_current_settings().brightness;

  if(state != ScreenState::On && brightness > dim_brightness)
    brightness = dim_brightness;

  set_display_brightness(brightness);
}

void update_screen_sleep(uint32_t time) {
  auto &buttons = blit::api_data.buttons;
  uint32_t state_now = buttons.state;

  bool active = state_now != last_buttons || state_now != 0 || joystick_moved();
#if defined(AUTO_INSTALL) && defined(BUILD_LOADER)
  // keep the progress visible while installing
  active = active || installer_active();
#endif

  last_buttons = state_now;
  last_joystick = blit::api_data.joystick;

  if(active) {
    if(state == ScreenState::Off)
      swallow_buttons = true;

    last_activity = time;
    set_state(ScreenState::On);
  }

  if(swallow_buttons) {
    if(state_now == 0)
      swallow_buttons = false;
    else
      buttons = 0;
  }

  uint32_t timeout_ms = uint32_t(get_current_settings().sleep_timeout) * 1000;
  if(!timeout_ms) {
    set_state(ScreenState::On);
    return;
  }

  uint32_t dim_ms = timeout_ms - (timeout_ms / 2 < dim_time_ms ? timeout_ms / 2 : dim_time_ms);
  uint32_t idle = time - last_activity;

  if(idle >= timeout_ms)
    set_state(ScreenState::Off);
  else if(idle >= dim_ms)
    set_state(ScreenState::Dimmed);
  else
    set_state(ScreenState::On);
}

bool is_screen_asleep() {
  return state == ScreenState::Off;
}
