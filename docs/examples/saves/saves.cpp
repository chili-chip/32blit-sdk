// Saves: keep a best score between sessions.
//
// Press A as many times as you can in 5 seconds. The best score is saved
// with write_save() and loaded again with read_save() the next time the
// game starts.
#include "32blit.hpp"

using namespace blit;

// Everything that is saved goes in one plain struct. Add a version number
// so a later build of the game can recognise (and upgrade) old saves.
struct SaveData {
  uint32_t version = 1;
  uint32_t best = 0;
  uint32_t games_played = 0;
};

SaveData save;

enum class State { ready, playing, done } state = State::ready;
uint32_t round_start = 0;
uint32_t score = 0;
bool new_best = false;

void save_now() {
  write_save(save);
}

void init() {
  set_screen_mode(ScreenMode::hires);

  // read_save() returns false when there is no save yet (or it is the wrong size)
  if(!read_save(save) || save.version != 1)
    save = SaveData();

  // called when the player quits to the home screen (or closes the window on desktop)
  on_quit = save_now;
}

void update(uint32_t time) {
  switch(state) {
    case State::ready:
      if(buttons.pressed & Button::A) {
        state = State::playing;
        round_start = time;
        score = 0;
      }
      break;

    case State::playing:
      if(buttons.pressed & Button::A)
        score++;

      if(time - round_start >= 5000) {
        state = State::done;
        save.games_played++;
        new_best = score > save.best;
        if(new_best)
          save.best = score;

        // save at the end of every round too: switching the console off can't be caught
        save_now();
      }
      break;

    case State::done:
      if(buttons.pressed & Button::A)
        state = State::ready;
      break;
  }

  // Y clears the save file
  if(buttons.pressed & Button::Y) {
    remove_save();
    save = SaveData();
  }
}

void render(uint32_t time) {
  screen.pen = Pen(20, 20, 30);
  screen.clear();

  screen.pen = Pen(255, 255, 255);
  screen.text("Best: " + std::to_string(save.best), fat_font, Point(4, 4));
  screen.text("Played: " + std::to_string(save.games_played), minimal_font, Point(4, 16));

  std::string message;
  switch(state) {
    case State::ready:
      message = "Press A to start";
      break;
    case State::playing:
      message = std::to_string(score) + "\n" + std::to_string(5 - (time - round_start) / 1000) + " s left";
      break;
    case State::done:
      message = "Score " + std::to_string(score) + (new_best ? "\nNew best!" : "") + "\nA: again";
      break;
  }
  screen.text(message, fat_font, Rect(0, 30, 128, 70), true, TextAlign::center_center);

  screen.pen = Pen(150, 150, 150);
  screen.text("Y: delete save", minimal_font, Point(4, 118));
}
