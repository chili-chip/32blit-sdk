#pragma once

#include <functional>
#include <string>

enum class Screen {
  home,
  info,       // one game: details, delete, delete save data
  settings,
  games,      // settings > games
  storage,    // settings > storage
  about       // settings > about
};

void go_to(Screen screen);

// game details, B returns to the given screen
void open_info(int game_index, Screen back);

// A runs on_yes, B runs on_no, both close the dialog
void show_confirm(const std::string &message, std::function<void()> on_yes,
                  const char *yes_label = "Yes", const char *no_label = "No", std::function<void()> on_no = nullptr);
// any of A or B closes it
void show_message(const std::string &message);

// home screen carousel moves to this game next time it's shown
void select_game(int game_index);
