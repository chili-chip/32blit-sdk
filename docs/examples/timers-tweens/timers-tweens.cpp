// Timers and tweens.
//
// A Timer calls a function after a delay, optionally repeating. A Tween
// moves a value from one number to another over time along a curve. Both
// are updated by the engine, you only start them.
#include "32blit.hpp"

using namespace blit;

Timer blink_timer;
Timer spawn_timer;
Tween slide;
Tween bounce;

bool blink_on = true;
int spawned = 0;

void blink(Timer &timer) {
  blink_on = !blink_on;
}

void spawn(Timer &timer) {
  spawned++;
  // timer.loop_count counts how many times it has fired
}

void init() {
  set_screen_mode(ScreenMode::hires);

  // fire every 500 ms, forever (loops = -1)
  blink_timer.init(blink, 500, -1);
  blink_timer.start();

  // fire every second, five times, then stop
  spawn_timer.init(spawn, 1000, 5);
  spawn_timer.start();

  // 0 to 100 over 2 seconds, repeating
  slide.init(tween_ease_in_out_quad, 0.0f, 100.0f, 2000, -1);
  slide.start();

  // a sine wave between 20 and 60, one cycle per 1.5 seconds
  bounce.init(tween_sine, 20.0f, 60.0f, 1500, -1);
  bounce.start();
}

void update(uint32_t time) {
  // A pauses and resumes the slide
  if(buttons.pressed & Button::A) {
    if(slide.is_running())
      slide.pause();
    else
      slide.start();
  }

  // B restarts the spawn timer
  if(buttons.pressed & Button::B) {
    spawned = 0;
    spawn_timer.stop();
    spawn_timer.start();
  }
}

void render(uint32_t time) {
  screen.pen = Pen(20, 20, 30);
  screen.clear();

  screen.pen = Pen(255, 255, 255);
  screen.text("Tween (A: pause)", minimal_font, Point(4, 4));
  screen.pen = Pen(214, 40, 40);
  screen.circle(Point(14 + int(slide.value), 22), 6);

  screen.pen = Pen(70, 170, 60);
  screen.circle(Point(100, 30 + int(bounce.value)), 6);

  screen.pen = Pen(255, 255, 255);
  screen.text("Timers (B: restart)", minimal_font, Point(4, 40));
  if(blink_on) {
    screen.pen = Pen(250, 210, 60);
    screen.rectangle(Rect(4, 52, 10, 10));
  }

  screen.pen = Pen(70, 120, 220);
  for(int i = 0; i < spawned; i++)
    screen.rectangle(Rect(4 + i * 14, 70, 10, 10));

  screen.pen = Pen(200, 200, 200);
  screen.text(spawn_timer.is_finished() ? "spawn timer finished" : "spawning...", minimal_font, Point(4, 86));
}
