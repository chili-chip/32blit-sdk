# Particles

`engine/particle.hpp`

A simple particle system for steady streams of particles: fountains, smoke, sparks from an engine. The [animation tutorial](../tutorials/animation.md#particles) has a complete example.

## `Particle`

```cpp
struct Particle {
  Vec2 pos;             // position
  Vec2 vel;             // velocity in units per second
  float age;            // 0.0 when created, 1.0 at the end of its life
  uint32_t age_ms = 0;  // age in milliseconds

  Particle(Vec2 pos, Vec2 vel);
};
```

To store more per particle, such as a colour or a sprite, derive from it and return your own type from the generate function, then cast back when drawing:

```cpp
struct Spark : Particle {
  Pen colour;
  Spark(Vec2 pos, Vec2 vel, Pen colour) : Particle(pos, vel), colour(colour) {}
};
```

## `ParticleGenerator`

```cpp
struct ParticleGenerator {
  uint32_t count;
  uint32_t lifetime_ms;
  Vec2 force;
  std::function<Particle *(void)> generate;
  std::deque<Particle *> particles;

  ParticleGenerator(uint32_t count, uint32_t lifetime_ms,
                    std::function<Particle *(void)> generate);
  ~ParticleGenerator();

  void update(uint32_t time_ms);
};
```

`ParticleGenerator(count, lifetime_ms, generate)`
:   `count` is how many particles to have alive once the stream is going, `lifetime_ms` how long each lives, and `generate` a function that returns a **new** particle (`new Particle(...)`). The generator owns the particles and deletes them.

`force`
:   A constant acceleration added to every particle's velocity, in units per second per second. `Vec2(0, 90)` is a reasonable gravity at 128×128. It starts at zero.

`particles`
:   The live particles, oldest first. Loop over it to draw them.

`update(time_ms)`
:   Advances the system to `time_ms`, which must be the **current time** (pass `update()`'s `time`), not the time since the last call. It:

    1. deletes particles older than `lifetime_ms`;
    2. adds `force` to each particle's velocity, moves it by its velocity, and updates `age` and `age_ms`;
    3. calls `generate` until there are as many particles as the age of the oldest one calls for, which spreads `count` particles evenly over a lifetime.

The stream starts with one particle and builds up to `count` over the first `lifetime_ms`.

```cpp
Particle *new_smoke() {
  Vec2 vel(blit::random() % 20 - 10.0f, -30.0f);
  return new Particle(Vec2(64, 120), vel);
}

ParticleGenerator smoke(100, 3000, new_smoke);

void update(uint32_t time) {
  smoke.update(time);
}

void render(uint32_t time) {
  for(auto p : smoke.particles) {
    screen.pen = Pen(200, 200, 200, int(255 * (1.0f - p->age)));
    screen.circle(Point(p->pos), 1 + int(p->age * 4));
  }
}
```

The generate function can capture state, for example to move the emitter:

```cpp
Vec2 emitter(64, 64);
ParticleGenerator trail(60, 500, [] { return new Particle(emitter, Vec2(0, 20)); });
```

## One generator at a time

`update()` keeps the time of the previous call in a single variable shared by **all** generators. If two generators are updated in the same step, the second one sees no time pass, so its particles never move or age. In practice:

* use one `ParticleGenerator` in your game, and draw other effects with your own particle lists (a `std::vector` of structs, as in [Chili Catch](../tutorials/complete-game.md#sparks));
* the first call after the game starts moves nothing, because there's no previous time yet.

Each `Particle` is allocated separately on the heap, which is fine for a few hundred particles. For thousands, a fixed array of your own structs is faster and uses less memory on the console.
