#include <cstdio>
#include <cstring>

#include "hardware/clocks.h"
#include "hardware/i2c.h"
#include "hardware/structs/rosc.h"
#include "hardware/vreg.h"
#include "hardware/timer.h"
#include "pico/binary_info.h"
#include "pico/multicore.h"
#include "pico/rand.h"
#include "pico/unique_id.h"
#include "pico/stdlib.h"

#include "audio.hpp"
#include "binary_info.hpp"
#include "blit_launch.hpp"
#include "config.h"
#include "display.hpp"
#include "file.hpp"
#include "input.hpp"
#include "installer.hpp"
#include "led.hpp"
#include "multiplayer.hpp"
#include "power.hpp"
#include "settings.hpp"
#include "storage.hpp"
#include "system_menu.hpp"
#include "usb.hpp"

#include "engine/api_private.hpp"

using namespace blit;

static blit::AudioChannel channels[CHANNEL_COUNT];

int (*do_tick)(uint32_t time) = blit::tick;

static alarm_id_t home_hold_alarm_id = 0;
static bool home_held = false;

// holding HOME asks the main loop to quit, so the game can save first
static volatile bool quit_requested = false;
static alarm_id_t quit_timeout_alarm_id = 0;

// override terminate handler to save ~20-30k
namespace __cxxabiv1 {
  std::terminate_handler __terminate_handler = std::abort;
}

static uint32_t now() {
  return to_ms_since_boot(get_absolute_time());
}

static uint32_t random() {
	return get_rand_32();
}

static void debug(const char *message) {
  auto p = message;
  while(*p)
    putchar(*p++);
}

static uint32_t get_us_timer() {
  return to_us_since_boot(get_absolute_time());
}

static uint32_t get_max_us_timer() {
  return 0xFFFFFFFF; // it's a 64bit timer...
}

static GameMetadata get_metadata() {
  GameMetadata ret;

  // check for blit metadata
  if(auto meta = get_running_game_metadata()) {
    // this is identical to the 32blit-stm code
    ret.title = meta->title;
    ret.author = meta->author;
    ret.description = meta->description;
    ret.version = meta->version;

    if(memcmp(meta + 1, "BLITTYPE", 8) == 0) {
      auto type_meta = reinterpret_cast<RawTypeMetadata *>(reinterpret_cast<char *>(meta) + sizeof(*meta) + 8);
      ret.url = type_meta->url;
      ret.category = type_meta->category;
    } else {
      ret.url = "";
      ret.category = "none";
    }
    return ret;
  }

  // parse binary info
  extern binary_info_t *__binary_info_start, *__binary_info_end;

  for(auto tag_ptr = &__binary_info_start; tag_ptr != &__binary_info_end ; tag_ptr++) {
    if((*tag_ptr)->type != BINARY_INFO_TYPE_ID_AND_STRING)
      continue;

    auto id_str_tag = (binary_info_id_and_string_t *)*tag_ptr;

    if((*tag_ptr)->tag == BINARY_INFO_TAG_RASPBERRY_PI) {
      switch(id_str_tag->id) {
        case BINARY_INFO_ID_RP_PROGRAM_NAME:
          ret.title = id_str_tag->value;
          break;
        case BINARY_INFO_ID_RP_PROGRAM_VERSION_STRING:
          ret.version = id_str_tag->value;
          break;
        case BINARY_INFO_ID_RP_PROGRAM_URL:
          ret.url = id_str_tag->value;
          break;
        case BINARY_INFO_ID_RP_PROGRAM_DESCRIPTION:
          ret.description = id_str_tag->value;
          break;
      }
    } else if((*tag_ptr)->tag == BINARY_INFO_TAG_32BLIT) {
      switch(id_str_tag->id) {
        case BINARY_INFO_ID_32BLIT_AUTHOR:
          ret.author = id_str_tag->value;
          break;
        case BINARY_INFO_ID_32BLIT_CATEGORY:
          ret.category = id_str_tag->value;
          break;
      }
    }

  }
  return ret;
}

static bool get_system_info(SystemInfo &info) {
  info = SystemInfo();
  strncpy(info.firmware_version, get_version_string(), sizeof(info.firmware_version) - 1);
  pico_get_unique_board_id_string(info.serial, sizeof(info.serial));

#ifdef BUILD_LOADER
  get_app_flash_usage(info.app_flash_total, info.app_flash_used);
#endif
  get_storage_usage(info.storage_total, info.storage_free);
  return true;
}

static uint8_t *get_screen_data() {
  return screen.data;
}

// blit API
[[gnu::section(".rodata.api_const")]]
static const blit::APIConst blit_api_const {
  blit::api_version_major, blit::api_version_minor,

  ::channels,

  ::set_screen_mode,
  ::set_screen_palette,

  ::now,
  ::random,
  nullptr, // exit
  ::debug,

  ::open_file,
  ::read_file,
  ::write_file,
  ::close_file,
  ::get_file_length,
  ::list_files,
  ::file_exists,
  ::directory_exists,
  ::create_directory,
  ::rename_file,
  ::remove_file,
  ::get_save_path,
  ::is_fs_available,

  nullptr, // enable_us_timer
  ::get_us_timer,
  ::get_max_us_timer,

  nullptr, // decode_jpeg_buffer
  nullptr, // decode_jpeg_file

  ::launch_file,
  ::erase_game,
  ::get_type_handler_metadata,

  ::get_launch_path,

  ::is_multiplayer_connected,
  ::set_multiplayer_enabled,
  ::send_multiplayer_message,

  nullptr, // flash_to_tmp
  nullptr, // tmp_file_closed

  ::get_metadata,

  ::set_screen_mode_format,

  nullptr, // i2c_send
  nullptr, // i2c_recieve

  nullptr, // set_raw_cdc_enabled
  nullptr, // cdc_write
  nullptr, // cdc_read

  ::list_installed_games,
  ::can_launch,

  ::get_screen_data,
  ::set_framebuffer,

  ::get_battery_level,
  ::get_battery_voltage,
  ::is_usb_connected,
  ::get_system_settings,
  ::set_system_settings,

  ::get_system_info,
};

[[gnu::section(".bss.api_data")]]
static blit::APIData blit_api_data;

namespace blit {
  const APIConst &api = blit_api_const;
  APIData &api_data = blit_api_data;
}

// user funcs
void init();
void render(uint32_t);
void update(uint32_t);

void disable_user_code() {
  // TODO: handle re-enabling
  do_tick = blit::tick;
  blit::render = ::render;
}

void quit_to_launcher() {
  // let the game save before it's gone
  auto on_quit = api_data.on_quit;
  api_data.on_quit = nullptr;
  if(on_quit)
    on_quit();

  system_menu_reset();

  close_open_files();

  launch_pre_init();
  ::init(); // re-initialising the loader is effectively a reset
}

[[maybe_unused]]
static int64_t quit_timeout_callback(alarm_id_t id, void *user_data) {
  quit_timeout_alarm_id = 0;

  // the game hasn't returned to the main loop, quit without saving
  if(quit_requested) {
    quit_requested = false;
    api_data.on_quit = nullptr;
    quit_to_launcher();
  }

  return 0;
}

[[maybe_unused]]
static int64_t home_hold_callback(alarm_id_t id, void *user_data) {
  home_hold_alarm_id = 0;

  quit_requested = true;
  quit_timeout_alarm_id = add_alarm_in_ms(2000, quit_timeout_callback, nullptr, false);

  return 0;
}

static void handle_quit_request() {
  if(!quit_requested)
    return;

  // too late if the timeout already quit
  if(quit_timeout_alarm_id && !cancel_alarm(quit_timeout_alarm_id))
    return;

  quit_timeout_alarm_id = 0;
  quit_requested = false;
  quit_to_launcher();
}

static void check_home_button() {
#ifdef BUILD_LOADER
  if((api_data.buttons & Button::HOME) && !home_held) {
    // start timer for exit/reset
    home_held = true;
    home_hold_alarm_id = add_alarm_in_ms(1000, home_hold_callback, nullptr, false);
    debugf("home down at %i alarm %i\n", ::now(), home_hold_alarm_id);
  } else if(!(api_data.buttons & Button::HOME) && home_held) {
    home_held = false;

    // released before the timer fired: a short press
    if(home_hold_alarm_id) {
      debugf("home up at %i alarm %i\n", ::now(), home_hold_alarm_id);
      cancel_alarm(home_hold_alarm_id);
      home_hold_alarm_id = 0;

#ifdef SYSTEM_MENU
      system_menu_toggle();
#endif
    }
  }
#endif
}

bool core1_started = false;

#ifdef ENABLE_CORE1
// MUST be noinline: if this is inlined into core1_main the mix loop lives in
// XIP, core 1 hammers flash, and starting a game HardFaults core 0.
[[gnu::noinline, gnu::noreturn]]
static void __not_in_flash_func(core1_audio_loop)() {
  while(true) {
    update_audio(0);
    tight_loop_contents();
  }
}

void core1_main() {
  core1_started = true;
  multicore_lockout_victim_init();

  init_display_core1();
  init_audio();

  core1_audio_loop();
}

#else
static void alarm_callback(uint alarm_num) {
  timer_hw->intr = 1 << alarm_num;
  hardware_alarm_set_target(alarm_num, make_timeout_time_ms(5));

  update_audio(::now());
}
#endif

static void init_i2c() {
  // multiple drivers need i2c, initialise it in one place if needed
#ifdef DEFAULT_I2C_CLOCK
  i2c_init(i2c_default, DEFAULT_I2C_CLOCK);
  gpio_set_function(PICO_DEFAULT_I2C_SDA_PIN, GPIO_FUNC_I2C);
  gpio_set_function(PICO_DEFAULT_I2C_SCL_PIN, GPIO_FUNC_I2C);

  gpio_pull_up(PICO_DEFAULT_I2C_SDA_PIN);
  gpio_pull_up(PICO_DEFAULT_I2C_SCL_PIN);
#endif
}

int main() {
#if OVERCLOCK_250
#ifndef PICO_RP2350
  // Apply a modest overvolt, default is 1.10v.
  // this is required for a stable 250MHz on some RP2040s
  vreg_set_voltage(VREG_VOLTAGE_1_20);
  sleep_ms(10);
#endif

  set_sys_clock_khz(250000, false);
#endif

  init_usb();
  stdio_init_all();

  init_i2c();

  init_led();
  init_display();
  init_input();
  init_power();
  init_fs();
  init_settings();
#if !defined(ENABLE_CORE1)
  init_audio();
#endif

#if defined(ENABLE_CORE1)
  multicore_launch_core1(core1_main);
#else
  // fallback audio timer if core1 is unavailable / not enabled
  int alarm_num = hardware_alarm_claim_unused(true);
  hardware_alarm_set_callback(alarm_num, alarm_callback);
  hardware_alarm_set_target(alarm_num, make_timeout_time_ms(5));
#ifdef PICO_RP2350
  irq_set_priority(TIMER0_IRQ_0 + alarm_num, PICO_LOWEST_IRQ_PRIORITY);
#else
  irq_set_priority(TIMER_IRQ_0 + alarm_num, PICO_LOWEST_IRQ_PRIORITY);
#endif
#endif

#ifdef BUILD_LOADER
  create_type_handler_list();
#else
  blit::set_screen_mode(ScreenMode::lores);
#endif

  blit::render = ::render;
  blit::update = ::update;

  // user init
  ::init();

  while(true) {
    auto now = ::now();
    update_display(now);

    update_input();
    check_home_button();

    int ms_to_next_update = do_tick(::now());
    handle_quit_request();

    update_led();
    update_power(now);
    update_usb();
    update_fs();
#if defined(AUTO_INSTALL) && defined(BUILD_LOADER)
    // install .blit files copied onto the drive
    update_installer();
#endif

    // store settings changed through the API or the system menu
    if(!system_menu_is_open())
      save_settings();
    update_multiplayer();

    // do requested launch when no user code is running
    delayed_launch();

    if(ms_to_next_update > 1 && !display_render_needed())
      best_effort_wfe_or_timeout(make_timeout_time_ms(ms_to_next_update - 1));
  }

  return 0;
}
