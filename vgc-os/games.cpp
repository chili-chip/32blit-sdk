#include <algorithm>
#include <cstring>

#include "games.hpp"

#include "engine/api_private.hpp"
#include "executable.hpp"

using namespace blit;

static constexpr uint32_t game_block_size = 64 * 1024;

// most recently played first, stored in the home screen's save
struct PlayHistory {
  static constexpr uint32_t expected_magic = 0x56484953; // "SIHV"
  static constexpr int max_entries = 32;

  uint32_t magic = expected_magic;
  uint32_t count = 0;
  char titles[max_entries][25] = {};
};

static constexpr int history_slot = 0;

static std::vector<Game> games;
static int num_home_games = 0;
static std::vector<Surface *> covers;
static std::vector<bool> cover_tried;

std::vector<Game> &get_games() {
  return games;
}

int home_game_count() {
  return num_home_games;
}

std::vector<int> installed_software() {
  // everything: the home screen's games (installed ones and the files apps play), then the apps
  std::vector<int> ret(games.size());
  for(int i = 0; i < int(games.size()); i++)
    ret[i] = i;
  return ret;
}

template<size_t N>
static std::string fixed_string(const char (&buf)[N]) {
  return std::string(buf, std::find(buf, buf + N, '\0'));
}

static std::string lower(std::string s) {
  for(auto &c : s)
    c = tolower(c);
  return s;
}

static bool read_metadata(const char *meta, uint16_t len, Game &game, bool &is_launcher) {
  if(len < sizeof(RawMetadata))
    return false;

  auto raw = reinterpret_cast<const RawMetadata *>(meta);
  game.title = fixed_string(raw->title);
  game.author = fixed_string(raw->author);
  game.version = fixed_string(raw->version);
  game.description = fixed_string(raw->description);

  is_launcher = false;
  uint16_t off = sizeof(RawMetadata);
  if(off + 8 + sizeof(RawTypeMetadata) <= len && memcmp(meta + off, "BLITTYPE", 8) == 0) {
    auto type = reinterpret_cast<const RawTypeMetadata *>(meta + off + 8);
    is_launcher = strncmp(type->category, "launcher", sizeof(type->category)) == 0;
    game.is_app = strncmp(type->category, "handler", sizeof(type->category)) == 0;

    if(game.is_app && off + 8 + sizeof(RawTypeMetadata) + type->num_filetypes * 5 <= len) {
      for(int i = 0; i < type->num_filetypes; i++)
        game.filetypes.push_back(lower(fixed_string(type->filetypes[i])));
    }
  }

  return true;
}

// pointer to the metadata after "BLITMETA" and its length, or null
static const char *find_metadata(const uint8_t *data, uint32_t size, uint16_t &len) {
  if(size < sizeof(BlitGameHeader))
    return nullptr;

  auto header = reinterpret_cast<const BlitGameHeader *>(data);
  if(header->magic != blit_game_magic || header->end + 10 > size)
    return nullptr;

  auto meta = reinterpret_cast<const char *>(data + header->end);
  if(memcmp(meta, "BLITMETA", 8) != 0)
    return nullptr;

  memcpy(&len, meta + 8, 2);
  if(header->end + 10 + len > size)
    return nullptr;

  return meta + 10;
}

static void add_installed_games() {
  api.list_installed_games([](const uint8_t *ptr, uint32_t block, uint32_t size) {
    uint16_t len = 0;
    auto meta = find_metadata(ptr, size, len);

    Game game;
    bool is_launcher = false;
    if(meta && !read_metadata(meta, len, game, is_launcher))
      meta = nullptr;

    // the home screen itself and other launchers aren't games
    if(is_launcher)
      return;

    if(!meta)
      game.title = "Game " + std::to_string(block);

    game.path = "flash:/" + std::to_string(block) + ".blit";
    game.offset = block * game_block_size;
    game.size = size;
    game.installed = true;
    game.meta = meta;
    game.meta_len = meta ? len : 0;
    games.push_back(std::move(game));
  });
}

// without installed games (the desktop build), show the .blit files in storage
static void add_files() {
  for(auto &file : list_files("/")) {
    if(file.flags & FileFlags::directory || file.name.size() < 6 || file.name.compare(file.name.size() - 5, 5, ".blit") != 0)
      continue;

    File f(file.name);
    if(!f.is_open())
      continue;

    std::vector<char> data(f.get_length());
    f.read(0, data.size(), data.data());

    uint16_t len = 0;
    auto meta = find_metadata(reinterpret_cast<uint8_t *>(data.data()), data.size(), len);
    if(!meta)
      continue;

    Game game;
    bool is_launcher = false;
    if(!read_metadata(meta, len, game, is_launcher) || is_launcher)
      continue;

    game.path = file.name;
    game.size = file.size;
    game.meta_copy.assign(meta, meta + len);
    game.meta_len = len;
    games.push_back(std::move(game));
  }

  for(auto &game : games)
    game.meta = game.meta_copy.data();
}

// files on the drive that an installed app plays, at the top or one folder down (bitsy-games/...)
static void add_app_files() {
  std::vector<Game> files;

  auto app_for = [](const std::string &name) -> const Game * {
    auto dot = name.find_last_of('.');
    if(name.empty() || name[0] == '.' || dot == std::string::npos)
      return nullptr;

    auto ext = lower(name.substr(dot + 1));
    for(auto &game : games) {
      if(!game.is_app)
        continue;
      // like the loader, which compares the first 4 characters: "bits" plays .bitsy
      for(auto &type : game.filetypes) {
        if(!type.empty() && ext.compare(0, 4, type) == 0)
          return &game;
      }
    }
    return nullptr;
  };

  auto add_dir = [&](const std::string &dir) {
    for(auto &file : list_files(dir.empty() ? "/" : dir)) {
      if(file.flags & FileFlags::directory)
        continue;

      auto app = app_for(file.name);
      if(!app)
        continue;

      Game game;
      game.title = file.name.substr(0, file.name.find_last_of('.'));
      game.author = app->title;
      game.description = "Plays with " + app->title + ".";
      game.path = dir.empty() ? file.name : dir + "/" + file.name;
      game.size = file.size;
      game.is_app_file = true;
      files.push_back(std::move(game));
    }
  };

  bool any_apps = std::any_of(games.begin(), games.end(), [](const Game &game) { return game.is_app; });
  if(!any_apps)
    return;

  add_dir("");
  for(auto &dir : list_files("/")) {
    // .blit (saves) and .system are ours
    if(dir.flags & FileFlags::directory && !dir.name.empty() && dir.name[0] != '.')
      add_dir(dir.name);
  }

  std::sort(files.begin(), files.end(), [](const Game &a, const Game &b) { return a.title < b.title; });

  for(auto &file : files)
    games.push_back(std::move(file));
}

void load_games() {
  for(auto cover : covers) {
    if(cover) {
      delete[] cover->data;
      delete[] cover->palette;
      delete cover;
    }
  }

  games.clear();

  if(api.list_installed_games)
    add_installed_games();
  else
    add_files();

  add_app_files();

  PlayHistory history;
  if(!read_save(history, history_slot) || history.magic != PlayHistory::expected_magic)
    history = PlayHistory();

  // position in the history, never played sorts first
  auto rank = [&history](const Game &game) {
    for(uint32_t i = 0; i < history.count && i < PlayHistory::max_entries; i++) {
      if(game.title == history.titles[i])
        return int(i);
    }
    return -1;
  };

  std::stable_sort(games.begin(), games.end(), [&rank](const Game &a, const Game &b) {
    return rank(a) < rank(b);
  });

  // apps aren't on the home screen, keep them at the end for Settings > Installed software
  std::stable_partition(games.begin(), games.end(), [](const Game &game) { return !game.is_app; });
  num_home_games = std::count_if(games.begin(), games.end(), [](const Game &game) { return !game.is_app; });

  covers.assign(games.size(), nullptr);
  cover_tried.assign(games.size(), false);
}

static Surface *decode_splash(const Game &game) {
  auto meta = game.meta;
  auto len = game.meta_len;
  if(!meta)
    return nullptr;

  uint32_t off = sizeof(RawMetadata);
  if(off + 8 + sizeof(RawTypeMetadata) <= len && memcmp(meta + off, "BLITTYPE", 8) == 0) {
    auto type = reinterpret_cast<const RawTypeMetadata *>(meta + off + 8);
    off += 8 + sizeof(RawTypeMetadata) + type->num_filetypes * 5;
  }

  // icon, then splash
  for(int i = 0; i < 2; i++) {
    if(off + sizeof(packed_image) > len || memcmp(meta + off, "SPRITE", 6) != 0)
      return nullptr;

    packed_image image;
    memcpy(&image, meta + off, sizeof(image));
    if(off + image.byte_count > len)
      return nullptr;

    if(i == 1)
      return Surface::load(reinterpret_cast<const uint8_t *>(meta + off));

    off += image.byte_count;
  }

  return nullptr;
}

Surface *get_cover(int index) {
  if(index < 0 || index >= int(games.size()))
    return nullptr;

  if(!cover_tried[index]) {
    cover_tried[index] = true;
    covers[index] = decode_splash(games[index]);
  }

  return covers[index];
}

void trim_covers(int keep_index, int keep_range) {
  for(int i = 0; i < int(covers.size()); i++) {
    if(std::abs(i - keep_index) <= keep_range || !covers[i])
      continue;

    delete[] covers[i]->data;
    delete[] covers[i]->palette;
    delete covers[i];
    covers[i] = nullptr;
    cover_tried[i] = false;
  }
}

bool play_game(int index) {
  if(index < 0 || index >= int(games.size()))
    return false;

  auto &game = games[index];

  // move to the front of the history
  PlayHistory history;
  if(!read_save(history, history_slot) || history.magic != PlayHistory::expected_magic)
    history = PlayHistory();

  PlayHistory updated;
  strncpy(updated.titles[0], game.title.c_str(), sizeof(updated.titles[0]) - 1);
  updated.count = 1;

  for(uint32_t i = 0; i < history.count && i < PlayHistory::max_entries && updated.count < PlayHistory::max_entries; i++) {
    if(game.title != history.titles[i])
      memcpy(updated.titles[updated.count++], history.titles[i], sizeof(history.titles[i]));
  }

  write_save(updated, history_slot);

  return api.launch(game.path.c_str());
}

bool delete_game(int index) {
  if(index < 0 || index >= int(games.size()))
    return false;

  auto &game = games[index];

  if(game.installed)
    api.erase_game(game.offset);
  else
    remove_file(game.path);

  load_games();
  return true;
}

static std::string save_dir(const std::string &title) {
  return ".blit/" + title;
}

bool has_save_data(const std::string &title) {
  auto dir = save_dir(title);
  return directory_exists(dir) && !list_files(dir).empty();
}

bool has_save_data(int index) {
  if(index < 0 || index >= int(games.size()))
    return false;

  return has_save_data(games[index].title);
}

bool delete_save_data(const std::string &title) {
  auto dir = save_dir(title);
  if(!directory_exists(dir))
    return true;

  bool ok = true;
  for(auto &file : list_files(dir))
    ok = remove_file(dir + "/" + file.name) && ok;

  return remove_file(dir) && ok;
}

bool delete_save_data(int index) {
  if(index < 0 || index >= int(games.size()))
    return false;

  return delete_save_data(games[index].title);
}
