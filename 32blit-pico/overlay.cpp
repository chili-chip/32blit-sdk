#include "display.hpp"

#include "engine/engine.hpp"

#include "theme.hpp"

using namespace blit;

static void(*orig_render)(uint32_t) = nullptr;

static std::string message;
static uint32_t progress_value = 0, progress_total = 0;
static bool render_flag = false;

static void overlay_render(uint32_t time) {
  // TODO: don't want to do this if we add a menu to avoid the problems the stm32 firmware has...
  if(orig_render)
    orig_render(time);

  if(!message.empty() || progress_total) {
    const int w = screen.bounds.w, h = screen.bounds.h;
    // a panel at the bottom, its shadow inside the screen
    Rect panel(4, h - 32, w - 12, 26);
    draw_panel(panel);

    screen.pen = theme::text;
    screen.text(fit_text(message, panel.w - 16), minimal_font, Point(panel.x + 8, panel.y + 6));

    if(progress_total)
      draw_bar(Rect(panel.x + 8, panel.y + panel.h - 8, panel.w - 16, 3), progress_value, progress_total);
  }

  render_flag = false;
}

void set_render_overlay_enabled(bool enabled) {
  if(enabled && blit::render != overlay_render) {
    orig_render = blit::render;
    blit::render = overlay_render;
  } else if(!enabled && orig_render) {
    blit::render = orig_render;
    orig_render = nullptr;
  }
}

bool is_render_overlay_enabled() {
  return orig_render != nullptr;
}

void set_overlay_message(std::string_view text) {
  message = text;
}

void set_overlay_progress(uint32_t value, uint32_t total) {
  progress_value = value;
  progress_total = total;
}

void overlay_try_render(bool force) {
  render_flag = force;
  do {
    update_display(blit::now());
  } while(render_flag);
}
