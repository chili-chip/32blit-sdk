#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "32blit.hpp"

struct Game {
  std::string title, author, version, description;
  std::string path;     // what to pass to launch(): "flash:/<block>.blit" for installed games
  uint32_t offset = 0;  // flash offset, for erase_game
  uint32_t size = 0;    // bytes
  bool installed = false;

  // an installed app with category "handler" plays files of its filetypes and isn't shown on the
  // home screen itself, only in Settings > Games
  bool is_app = false;
  std::vector<std::string> filetypes;
  // a file that an app plays, rather than installed software
  bool is_app_file = false;

  // packed metadata, read lazily for the cover
  const char *meta = nullptr;
  uint16_t meta_len = 0;
  std::vector<char> meta_copy; // when the game isn't in flash
};

// installed games and files that an app plays, never played (newest installs) first, then most
// recently played, then the apps
std::vector<Game> &get_games();
// how many of get_games() go on the home screen: everything but the apps, which come last
int home_game_count();
// indices into get_games() of installed software (games and apps, not the files apps play),
// for Settings > Installed software
std::vector<int> installed_software();
void load_games();

// splash image of a game, decoded on first use and cached for nearby games
blit::Surface *get_cover(int index);
void trim_covers(int keep_index, int keep_range);

// remember that the game was played and start it
bool play_game(int index);

bool delete_game(int index);
// saves live in .blit/<title>/, so they outlive the game
bool delete_save_data(int index);
bool delete_save_data(const std::string &title);
bool has_save_data(int index);
bool has_save_data(const std::string &title);
