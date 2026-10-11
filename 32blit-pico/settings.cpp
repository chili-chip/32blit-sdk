#include <cstring>

#include "ff.h"

#include "settings.hpp"
#include "display.hpp"

#include "audio/audio.hpp"

static const char *settings_dir = ".system";
static const char *settings_path = ".system/settings.bin";
static const uint32_t settings_magic = 0x54455342; // "BSET"

struct StoredSettings {
  uint32_t magic;
  blit::SystemSettings settings;
};

static blit::SystemSettings current, stored;

static blit::SystemSettings clamp(blit::SystemSettings settings) {
  if(settings.brightness > 15)
    settings.brightness = 15;
  if(settings.volume > 100)
    settings.volume = 100;
  return settings;
}

void init_settings() {
  FIL file;
  StoredSettings data;
  UINT read = 0;

  if(f_open(&file, settings_path, FA_READ) == FR_OK) {
    if(f_read(&file, &data, sizeof(data), &read) != FR_OK)
      read = 0;
    f_close(&file);
  }

  if(read == sizeof(data) && data.magic == settings_magic)
    current = clamp(data.settings);

  stored = current;
  apply_settings(current);
}

const blit::SystemSettings &get_current_settings() {
  return current;
}

void apply_settings(const blit::SystemSettings &settings) {
  current = clamp(settings);

  set_display_brightness(current.brightness);
  unmute();
}

void unmute() {
  // square the percentage so the steps sound even
  blit::volume = uint32_t(current.volume) * current.volume * 0xFFFF / (100 * 100);
}

void save_settings() {
  if(memcmp(&current, &stored, sizeof(current)) == 0)
    return;

  f_mkdir(settings_dir);

  FIL file;
  if(f_open(&file, settings_path, FA_WRITE | FA_CREATE_ALWAYS) != FR_OK)
    return;

  StoredSettings data{settings_magic, current};
  UINT written = 0;
  f_write(&file, &data, sizeof(data), &written);
  f_close(&file);

  if(written == sizeof(data))
    stored = current;
}

bool get_system_settings(blit::SystemSettings &settings) {
  settings = current;
  return true;
}

bool set_system_settings(const blit::SystemSettings &settings) {
  apply_settings(settings);
  save_settings();
  return true;
}
