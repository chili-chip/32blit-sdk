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

  // packed metadata, read lazily for the cover
  const char *meta = nullptr;
  uint16_t meta_len = 0;
  std::vector<char> meta_copy; // when the game isn't in flash
};

// installed games, never played (newest installs) first, then most recently played
std::vector<Game> &get_games();
void load_games();

// splash image of a game, decoded on first use and cached for nearby games
blit::Surface *get_cover(int index);
void trim_covers(int keep_index, int keep_range);

// remember that the game was played and start it
bool play_game(int index);

bool delete_game(int index);
bool delete_save_data(int index);
bool has_save_data(int index);
