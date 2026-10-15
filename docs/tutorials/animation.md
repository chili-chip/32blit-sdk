# 7. Timers, tweens and particles

Games are full of things that happen over time: a power-up that runs out, a menu that slides in, sparks that fly and fade. The SDK has three helpers for these.

## Timers

![Timers and tweens](../assets/screenshots/timers-tweens.png){ .screenshot align=right }

A `Timer` calls a function after a delay, once, a set number of times, or forever:

```cpp
Timer blink_timer;

void blink(Timer &timer) {
  blink_on = !blink_on;
}

void init() {
  blink_timer.init(blink, 500, -1);   // every 500 ms, forever
  blink_timer.start();
}
```

The arguments to `init()` are the callback, the interval in milliseconds, and how many times to fire (`-1` for forever). The engine checks running timers before each frame and calls the callback when one is due; you don't need to update them yourself.

| Call | Effect |
| ---- | ------ |
| `start()` | start, or resume after `pause()` |
| `pause()` | stop counting; `start()` carries on from where it was |
| `stop()` | stop; `start()` begins again from zero |
| `is_running()`, `is_paused()`, `is_stopped()`, `is_finished()` | check the state |

`timer.loop_count` counts how many times it has fired. A timer with a set number of loops ends up `is_finished()`.

!!! note "Timers must outlive their use"
    The engine keeps a pointer to each started timer, so make timers global variables or members of objects that live as long as they run. A timer destroyed while running is removed safely, but a local `Timer` in a function stops as soon as the function returns.

## Tweens

A `Tween` moves a number from one value to another over a duration, following a curve. Read `tween.value` whenever you need it:

```cpp
Tween slide;

void init() {
  slide.init(tween_ease_in_out_quad, 0.0f, 100.0f, 2000, -1);
  slide.start();
}

void render(uint32_t time) {
  screen.circle(Point(14 + int(slide.value), 22), 6);
}
```

The arguments are the curve, the start and end values, the duration in milliseconds, and the number of loops (`-1` repeats forever, `1` runs once). Tweens have the same `start()`, `pause()`, `stop()` and `is_...()` calls as timers.

| Curve | Shape |
| ----- | ----- |
| `tween_linear` | constant speed |
| `tween_ease_in_quad` | starts fast and slows down to the end value |
| `tween_ease_out_quad` | starts slowly and speeds up |
| `tween_ease_in_out_quad` | slow at both ends, fast in the middle |
| `tween_sine` | starts at the *end* value, swings to the start value and back once per loop: good for bobbing and pulsing |

(The two one-sided quad curves are named the opposite way round from the usual easing names. The table describes what they actually do.)

You can write your own curve: any function `float f(uint32_t t, float from, float to, uint32_t duration)` that returns `from` at `t = 0` and `to` at `t = duration`.

Here is the example that shows timers and tweens together:

```cpp title="timers-tweens.cpp"
--8<-- "timers-tweens/timers-tweens.cpp"
```

## Particles

![Particles](../assets/screenshots/particles.png){ .screenshot align=right }

A `ParticleGenerator` manages a stream of particles: it creates them with your function, moves them, applies a constant force such as gravity, and removes them when they're too old.

```cpp title="particles.cpp"
--8<-- "particles/particles.cpp"
```

* The constructor takes how many particles to keep alive, how long each lives in milliseconds, and a function that returns a `new Particle(position, velocity)`.
* `force` is added to every particle's velocity each second; set `y` for gravity.
* Call `update(time)` with the current time every update. It works out how much time has passed since the last call itself.
* Each particle has `pos`, `vel`, and `age` (0 when new, 1 when about to be removed), which is handy for fading.

The generator spreads new particles evenly over the lifetime, so you get a steady stream, which suits fountains, smoke and engine trails. For a one-off burst, such as sparks when something explodes, a small `std::vector` of your own structs is simpler: see how [Chili Catch](complete-game.md#sparks) does it.

!!! warning "One generator at a time"
    `ParticleGenerator::update` remembers the previous time in one variable shared by every generator. With two generators updated in the same step, the second sees no time pass and its particles freeze. Use a single generator, or your own particle list for the others. See [Particles](../api/particles.md#one-generator-at-a-time).

## Try it

* Use a one-shot tween (loops `1`) to slide a "LEVEL 2" banner in from the left.
* Make the particle fountain change colour with `hsv_to_rgba()` as particles age.
* Use a timer with 3 loops for a "3, 2, 1, GO" countdown.

Next: [tile maps](tilemaps.md).
