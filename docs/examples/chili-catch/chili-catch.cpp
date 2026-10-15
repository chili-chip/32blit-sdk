// Chili Catch: a complete small game.
//
// Move with the d-pad and catch the falling chilis. Coins are worth more,
// hearts give a life back, and bombs take one away. The game speeds up as
// the score grows, and the best score is saved.
//
// It brings together the pieces from the tutorials: the game loop, input,
// sprites, sound, saves, tweens and simple particles.
#include <vector>

#include "32blit.hpp"
#include "assets.hpp"

using namespace blit;

// ---------------------------------------------------------------------------
// constants

constexpr int screen_w = 128, screen_h = 128;
constexpr int ground_y = 112;      // top of the grass
constexpr float player_speed = 1.2f; // pixels per update (update runs 100 times a second)
constexpr int start_lives = 3, max_lives = 5;

// sprite indices in examples/shared/sprites.png
enum Sprite : uint16_t {
  CHILI = 0, PLAYER_1 = 1, PLAYER_2 = 2, HEART = 3, COIN = 4, BOMB = 5, STAR = 6, BASKET = 7, GRASS = 8, DIRT = 9
};

// ---------------------------------------------------------------------------
// game state

// --8<-- [start:state]
enum class State { title, playing, game_over };

struct Item {
  Sprite sprite;
  Vec2 pos;
  float speed;
};

struct Spark {
  Vec2 pos, vel;
  uint32_t born;
};

struct SaveData {
  uint32_t version = 1;
  uint32_t best = 0;
};

State state = State::title;
SaveData save;

float player_x = 60;
bool facing_left = false;
bool moving = false;

std::vector<Item> items;
std::vector<Spark> sparks;
uint32_t score = 0;
int lives = start_lives;
uint32_t next_spawn = 0;
uint32_t hurt_until = 0;   // flash the player after a bomb
bool new_best = false;

Tween title_bob;
// --8<-- [end:state]

// ---------------------------------------------------------------------------
// sound

// --8<-- [start:sound]
void setup_sound() {
  // channel 0: pickup blip
  channels[0].waveforms = Waveform::SQUARE;
  channels[0].attack_ms = 2;
  channels[0].decay_ms = 80;
  channels[0].sustain = 0;
  channels[0].volume = 0x6000;

  // channel 1: bomb explosion
  channels[1].waveforms = Waveform::NOISE;
  channels[1].attack_ms = 5;
  channels[1].decay_ms = 400;
  channels[1].sustain = 0;

  // channel 2: game over / extra life tune
  channels[2].waveforms = Waveform::TRIANGLE;
  channels[2].attack_ms = 10;
  channels[2].decay_ms = 300;
  channels[2].sustain = 0;
}

void play(int channel, uint16_t frequency) {
  channels[channel].frequency = frequency;
  channels[channel].trigger_attack();
}
// --8<-- [end:sound]

// ---------------------------------------------------------------------------
// helpers

// --8<-- [start:helpers]
void save_best() {
  write_save(save);
}

void burst(Vec2 pos, int count) {
  for(int i = 0; i < count; i++) {
    float angle = (blit::random() % 628) / 100.0f;
    float speed = 0.3f + (blit::random() % 100) / 100.0f;
    sparks.push_back({pos, Vec2(cosf(angle) * speed, sinf(angle) * speed - 0.5f), now()});
  }
}

void start_game(uint32_t time) {
  state = State::playing;
  items.clear();
  sparks.clear();
  score = 0;
  lives = start_lives;
  player_x = 60;
  next_spawn = time + 500;
  new_best = false;
}

void end_game() {
  state = State::game_over;
  new_best = score > save.best;
  if(new_best) {
    save.best = score;
    save_best();
  }
  play(2, 196);
}

void spawn_item(uint32_t time) {
  // pick what falls: mostly chilis, some coins and bombs, the odd heart
  int roll = blit::random() % 100;
  Sprite sprite = CHILI;
  if(roll >= 97) sprite = HEART;
  else if(roll >= 75) sprite = BOMB;
  else if(roll >= 63) sprite = COIN;

  // everything falls faster as the score goes up
  float speed = 0.35f + std::min<uint32_t>(score, 200) / 250.0f + (blit::random() % 30) / 100.0f;

  items.push_back({sprite, Vec2(float(4 + blit::random() % 116), -8.0f), speed});

  // and arrives more often
  uint32_t gap = 900 - std::min<uint32_t>(score * 4, 600);
  next_spawn = time + gap / 2 + blit::random() % gap;
}
// --8<-- [end:helpers]

// ---------------------------------------------------------------------------
// init / update / render

// --8<-- [start:init]
void init() {
  set_screen_mode(ScreenMode::hires);
  screen.sprites = Surface::load(asset_sprites);

  if(!read_save(save) || save.version != 1)
    save = SaveData();
  on_quit = save_best;

  setup_sound();

  title_bob.init(tween_sine, -3.0f, 3.0f, 1200, -1);
  title_bob.start();
}
// --8<-- [end:init]

// --8<-- [start:playing]
void update_playing(uint32_t time) {
  moving = false;
  if(buttons & Button::DPAD_LEFT) {
    player_x -= player_speed;
    facing_left = true;
    moving = true;
  }
  if(buttons & Button::DPAD_RIGHT) {
    player_x += player_speed;
    facing_left = false;
    moving = true;
  }
  player_x = std::max(0.0f, std::min(player_x, float(screen_w - 16)));

  if(time >= next_spawn)
    spawn_item(time);

  // the basket is the 16 pixels in front of the player's hands
  Rect basket(int(player_x), ground_y - 14, 16, 6);

  for(auto it = items.begin(); it != items.end();) {
    it->pos.y += it->speed;

    Rect item_rect(Point(it->pos), Size(8, 8));
    bool caught = item_rect.intersects(basket);
    bool missed = it->pos.y > ground_y;

    if(caught) {
      switch(it->sprite) {
        case CHILI: score += 1; play(0, 880); break;
        case COIN:  score += 5; play(0, 1320); break;
        case HEART:
          lives = std::min(lives + 1, max_lives);
          play(2, 660);
          break;
        case BOMB:
          lives--;
          hurt_until = time + 600;
          play(1, 200);
          break;
        default: break;
      }
      burst(it->pos + Vec2(4, 4), it->sprite == BOMB ? 20 : 8);
    } else if(missed && it->sprite == CHILI) {
      // a dropped chili costs nothing, but gives a little puff
      burst(Vec2(it->pos.x + 4, ground_y), 3);
    }

    if(caught || missed)
      it = items.erase(it);
    else
      ++it;
  }

  if(lives <= 0)
    end_game();
}
// --8<-- [end:playing]

// --8<-- [start:update]
void update(uint32_t time) {
  switch(state) {
    case State::title:
      if(buttons.pressed & Button::A)
        start_game(time);
      break;

    case State::playing:
      update_playing(time);
      break;

    case State::game_over:
      if(buttons.pressed & Button::A)
        start_game(time);
      break;
  }

  // sparks fly outwards, fall, and fade after 600 ms
  for(auto &s : sparks) {
    s.pos += s.vel;
    s.vel.y += 0.03f;
  }
  sparks.erase(std::remove_if(sparks.begin(), sparks.end(),
    [time](const Spark &s) { return time - s.born > 600; }), sparks.end());
}
// --8<-- [end:update]

// --8<-- [start:render]
void draw_background() {
  for(int y = 0; y < ground_y; y++) {
    screen.pen = Pen(100 + y / 2, 160 + y / 4, 235);
    screen.h_span(Point(0, y), screen_w);
  }
  for(int x = 0; x < screen_w; x += 8) {
    screen.sprite(GRASS, Point(x, ground_y));
    screen.sprite(DIRT, Point(x, ground_y + 8));
  }
}

void draw_hud() {
  screen.pen = Pen(0, 0, 0, 120);
  screen.rectangle(Rect(0, 0, screen_w, 11));

  for(int i = 0; i < lives; i++)
    screen.sprite(HEART, Point(2 + i * 9, 2));

  screen.pen = Pen(255, 255, 255);
  screen.text(std::to_string(score), minimal_font, Point(screen_w - 3, 2), true, TextAlign::top_right);
}

void draw_player(uint32_t time) {
  // flash while hurt
  if(time < hurt_until && (time / 80) % 2)
    return;

  uint16_t frame = moving && (time / 150) % 2 ? PLAYER_2 : PLAYER_1;
  uint8_t flip = facing_left ? SpriteTransform::HORIZONTAL : SpriteTransform::NONE;
  int x = int(player_x);

  // the player and the basket they hold out in front
  screen.sprite(frame, Point(facing_left ? x + 8 : x, ground_y - 8), flip);
  screen.sprite(BASKET, Point(facing_left ? x : x + 8, ground_y - 13));
}

void render(uint32_t time) {
  draw_background();

  for(auto &item : items)
    screen.sprite(item.sprite, Point(item.pos));

  if(state != State::title)
    draw_player(time);

  for(auto &s : sparks) {
    int age = time - s.born;
    screen.pen = Pen(255, 220, 80, 255 - age * 255 / 600);
    screen.pixel(Point(s.pos));
  }

  if(state == State::playing) {
    draw_hud();
    return;
  }

  // title and game over panels
  screen.pen = Pen(0, 0, 0, 150);
  screen.rectangle(Rect(10, 24, 108, 64));
  screen.pen = Pen(255, 255, 255);

  int bob = int(title_bob.value);
  if(state == State::title) {
    screen.sprite(CHILI, Point(56, 30 + bob), Point(4, 4), 2.0f);
    screen.text("CHILI CATCH", fat_font, Point(64, 48), true, TextAlign::center_center);
    screen.text("Best: " + std::to_string(save.best), minimal_font, Point(64, 62), true, TextAlign::center_center);
  } else {
    screen.text("GAME OVER", fat_font, Point(64, 36 + bob), true, TextAlign::center_center);
    screen.text("Score: " + std::to_string(score), minimal_font, Point(64, 54), true, TextAlign::center_center);
    screen.text(new_best ? "New best!" : "Best: " + std::to_string(save.best), minimal_font, Point(64, 64), true, TextAlign::center_center);
  }

  if((time / 500) % 2)
    screen.text("Press A", minimal_font, Point(64, 78), true, TextAlign::center_center);
}
// --8<-- [end:render]
