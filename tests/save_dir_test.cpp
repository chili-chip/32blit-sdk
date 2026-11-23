// Host test for blit::save_dir(): where a game's saves go on the drive.
#include "engine/save_dir.hpp"

#include <cstdio>
#include <string>

static int failures = 0;

static void check(const std::string &got, const std::string &want, const char *what) {
  if(got != want) {
    std::printf("FAIL %s: got \"%s\", want \"%s\"\n", what, got.c_str(), want.c_str());
    failures++;
  }
}

int main() {
  check(blit::save_dir("Snek"), ".blit/Snek/", "a game");
  check(blit::save_dir("Snek", ""), ".blit/Snek/", "no launch path");
  check(blit::save_dir("Chili RPG", "my-quest.rpg"), ".blit/Chili RPG/my-quest.rpg/", "a handler with a file");
  check(blit::save_dir("Chili RPG", "rpg-games/my-quest.rpg"), ".blit/Chili RPG/my-quest.rpg/", "a file in a folder");
  check(blit::save_dir("Chili RPG", "/my-quest.rpg"), ".blit/Chili RPG/my-quest.rpg/", "a path from the root");
  std::string long_name(100, 'a');
  check(blit::save_dir("Chili RPG", long_name + ".rpg"), ".blit/Chili RPG/" + std::string(64, 'a') + "/", "a long name is cut");
  check(blit::save_parent_dir(".blit/Chili RPG/my-quest.rpg/"), ".blit/Chili RPG/", "the app's folder");
  check(blit::save_parent_dir(".blit/Snek/"), ".blit/", "a game's parent");
  std::printf(failures ? "save_dir: %d failed\n" : "save_dir: ok\n", failures);
  return failures ? 1 : 0;
}
