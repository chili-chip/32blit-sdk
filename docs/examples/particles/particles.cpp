// Particles: a fountain made with ParticleGenerator.
//
// A generator owns a list of particles. It creates new ones with your
// generate function, moves them by their velocity plus a constant force,
// and removes them when they are older than their lifetime. It aims to keep
// `count` particles alive, spread evenly over the lifetime.
#include "32blit.hpp"

using namespace blit;

Particle *new_drop() {
  // start at the nozzle with a random upward velocity (pixels per second)
  float spread = (float(blit::random() % 200) - 100.0f) / 100.0f;
  float speed = 90.0f + float(blit::random() % 40);
  return new Particle(Vec2(64, 110), Vec2(spread * 25.0f, -speed));
}

// up to 300 particles, each living 2 seconds
ParticleGenerator fountain(300, 2000, new_drop);

void init() {
  set_screen_mode(ScreenMode::hires);

  // gravity, in pixels per second per second
  fountain.force = Vec2(0, 90);
}

void update(uint32_t time) {
  // pass the current time: the generator works out the time since its last update
  fountain.update(time);

  // A blows the spray sideways while held
  fountain.force.x = (buttons & Button::A) ? 60.0f : 0.0f;
}

void render(uint32_t time) {
  screen.pen = Pen(10, 15, 30);
  screen.clear();

  for(auto p : fountain.particles) {
    // p->age goes from 0 (new) to 1 (about to be removed)
    screen.pen = Pen(120, 180, 255, int(255 * (1.0f - p->age)));
    screen.pixel(Point(p->pos));
  }

  screen.pen = Pen(150, 150, 150);
  screen.rectangle(Rect(58, 110, 12, 18));

  screen.pen = Pen(255, 255, 255);
  screen.text(std::to_string(fountain.particles.size()) + " particles", minimal_font, Point(2, 2));
  screen.text("Hold A for wind", minimal_font, Point(2, 12));
}
