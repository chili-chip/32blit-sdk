# Timers and tweens

`engine/timer.hpp`, `engine/tweening.hpp`

Timers call a function after a delay; tweens move a number smoothly between two values. Both are driven by the engine before each round of updates, so you don't update them yourself. See the [animation tutorial](../tutorials/animation.md) for a walkthrough.

## `Timer`

```cpp
struct Timer {
  using TimerCallback = std::function<void(Timer &timer)>;

  TimerCallback callback = nullptr;
  uint32_t duration = 0;   // ms between calls
  uint32_t started = 0;    // when the current period started
  uint32_t paused = 0;     // when it was paused
  int32_t loops = -1;      // how many times to fire, -1 = forever
  int32_t loop_count = 0;  // how many times it has fired
  uint8_t state = UNINITIALISED;

  enum state { UNINITIALISED, STOPPED, RUNNING, PAUSED, FINISHED };

  Timer();
  Timer(TimerCallback callback, uint32_t duration, int32_t loops = -1);

  void init(TimerCallback callback, uint32_t duration, int32_t loops = -1);
  void start();
  void pause();
  void stop();

  bool is_running();
  bool is_paused();
  bool is_stopped();
  bool is_finished();
};
```

`init(callback, duration, loops)`
:   Sets what the timer calls, how many milliseconds apart, and how many times. `loops = 1` makes a one-shot timer; `-1` repeats until stopped. It doesn't start the timer.

`start()`
:   Starts the timer from now and resets `loop_count`. On a paused timer it resumes instead, keeping the time already elapsed. Calling it on a finished timer runs it again.

`pause()`
:   Pauses a running timer. `start()` resumes it.

`stop()`
:   Stops the timer. `start()` begins again from zero.

`callback`
:   Called with the timer itself, so one function can serve several timers and can stop or change the timer it was called for. On the last loop, the state is already `FINISHED` when it's called.

```cpp
Timer spawn_timer;

void spawn(Timer &t) {
  add_enemy();
  if(t.loop_count == 10)  // speed up after ten spawns
    t.duration = 500;
}

void init() {
  spawn_timer.init(spawn, 1000);  // every second, forever
  spawn_timer.start();
}
```

### How timing works

* Timers are checked once per pass of the engine's main loop, just before the pending `update()` calls, using [`now()`](engine.md#time). A timer fires on the first check after more than `duration` milliseconds have passed, so it can fire up to a frame late, and fires at most once per check: a 1 ms timer fires about once per frame, not 1000 times a second.
* After firing, the next period starts from the time of the check, so lateness isn't made up later.
* Timer callbacks run before `update()`, outside it, so they can change game state but shouldn't draw.

!!! warning "Keep timers alive while they run"
    `start()` registers the timer in a global list by pointer. Make timers globals or members of long-lived objects. A timer's destructor removes it from the list, but destroying a timer from inside any timer's callback (including deleting the object that owns it) changes the list while it's being walked; stop it instead and destroy it later.

## `Tween`

```cpp
struct Tween {
  using TweenFunction = std::function<float(uint32_t t, float b, float c, uint32_t d)>;

  TweenFunction function = nullptr;
  float from = 0.0f;
  float to = 1.0f;
  float value = 0.0f;      // the current value: read this
  uint32_t duration = 0;
  int32_t loops = -1;
  int32_t loop_count = 0;
  uint32_t started = 0;
  uint32_t paused = 0;
  uint8_t state = UNINITIALISED;

  enum state { UNINITIALISED, STOPPED, RUNNING, PAUSED, FINISHED };

  Tween();
  Tween(TweenFunction function, float start, float end, uint32_t duration, int32_t loops = -1);

  void init(TweenFunction function, float start, float end, uint32_t duration, int32_t loops = -1);
  void start();
  void pause();
  void stop();

  bool is_running();
  bool is_paused();
  bool is_stopped();
  bool is_finished();
};
```

`init(function, start, end, duration, loops)`
:   Sets the easing function, the start and end values, the length of one run in milliseconds, and how many runs (`1` once, `-1` forever).

`value`
:   The current value, updated by the engine while the tween runs. It stays at its last value when the tween is paused, stopped or finished.

`start()`, `pause()`, `stop()` and the `is_...()` checks work like the timer's. `start()` sets `value` to `from` immediately.

```cpp
Tween fade;

void init() {
  fade.init(tween_linear, 0.0f, 255.0f, 1000, 1);  // once, over a second
  fade.start();
}

void render(uint32_t time) {
  screen.alpha = uint8_t(fade.value);
  draw_logo();
  screen.alpha = 255;
}
```

On the last step of each run, `value` is computed from the actual elapsed time, which can be slightly past `duration`, so a linear tween can end a little beyond `to`. If you need the exact end value, set it yourself when `is_finished()` becomes true. A looping tween jumps back to `from` at the start of each run; for a smooth back-and-forth use `tween_sine`, or run from `a` to `b` and swap.

Like timers, tweens are kept in a global list by pointer, so keep them alive while they run.

## Easing functions

```cpp
float tween_linear(uint32_t t, float b, float c, uint32_t d);
float tween_ease_in_quad(uint32_t t, float b, float c, uint32_t d);
float tween_ease_out_quad(uint32_t t, float b, float c, uint32_t d);
float tween_ease_in_out_quad(uint32_t t, float b, float c, uint32_t d);
float tween_sine(uint32_t t, float b, float c, uint32_t d);
```

Each takes the elapsed time `t`, the start value `b`, the end value `c` and the duration `d`, and returns the value at that time.

| Function | Curve |
| -------- | ----- |
| `tween_linear` | straight line from start to end |
| `tween_ease_in_quad` | starts fast and slows down into the end value |
| `tween_ease_out_quad` | starts slowly and speeds up |
| `tween_ease_in_out_quad` | slow at both ends, fast in the middle |
| `tween_sine` | starts at the **end** value, swings to the start value at the halfway point, and back to the end |

!!! note "The quad names are swapped"
    In most easing libraries "ease in" means *starts slowly*. Here `tween_ease_in_quad` decelerates and `tween_ease_out_quad` accelerates. The table describes what they actually do; the names are kept for compatibility with existing games.

You can pass any function or lambda with the same signature:

```cpp
// ease out cubic: fast start, gentle stop
auto ease_cubic = [](uint32_t t, float b, float c, uint32_t d) {
  float f = 1.0f - float(t) / float(d);
  return b + (c - b) * (1.0f - f * f * f);
};

Tween drop(ease_cubic, -16.0f, 56.0f, 600, 1);
```

## Interpolation helpers

`math/interpolation.hpp`

```cpp
float lerp(float value, float start, float end);
Vec2  lerp(float value, Vec2 start, Vec2 end);
float lerp(float value, float start, float end, float min, float max);
Vec2  lerp(float value, float start, float end, Vec2 min, Vec2 max);
```

The three-argument forms behave differently for floats and vectors:

`lerp(value, start, end)` with floats
:   An *inverse* lerp: how far `value` is from `start` to `end`, as `0.0` to `1.0` (not clamped).

`lerp(t, start, end)` with `Vec2`s
:   A normal lerp: the point `t` of the way from `start` to `end` (`0.0` gives `start`).

`lerp(value, start, end, min, max)`
:   Maps `value` from the range `start`..`end` onto `min`..`max`, clamping it to the range first. If `start == end` it returns `min`.

For an ordinary float lerp, write it out: `a + (b - a) * t`.
