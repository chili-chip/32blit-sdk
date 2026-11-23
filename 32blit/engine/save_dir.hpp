#pragma once

#include <string>
#include <string_view>

namespace blit {

// The folder a game's saves go in, on the drive: ".blit/<title>/".
//
// An app that plays files (a handler, such as citsy or Chili RPG) is started
// with the file to play as its launch path. Its saves then go in a folder of
// their own per file: ".blit/<title>/<file name>/". So each game it plays
// keeps its own saves, and the home screen can delete them one game at a time.
// Long file names are cut to 64 characters.
inline std::string save_dir(std::string_view title, std::string_view launch_path = {}) {
  std::string dir = ".blit/" + std::string(title) + "/";

  auto slash = launch_path.find_last_of("/\\");
  auto name = slash == std::string_view::npos ? launch_path : launch_path.substr(slash + 1);
  if(!name.empty())
    dir += std::string(name.substr(0, 64)) + "/";

  return dir;
}

// The folder that holds a save folder, to create it first.
inline std::string save_parent_dir(std::string_view dir) {
  auto end = dir.find_last_of('/', dir.size() - 2);
  return std::string(dir.substr(0, end + 1));
}

}
