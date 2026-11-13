// The "game" side of sandbox_test: code in its own flash image, data in game
// RAM, reaching the loader only through the APIConst at 0x10000200, as a .blit does.
#include <cstdint>
#include <string>

#include "engine/api_private.hpp"

#define API (*reinterpret_cast<const blit::APIConst *>(0x10000200))
#define GAME extern "C" [[gnu::section(".game_text"), gnu::noinline]]

[[gnu::section(".game_bss")]] uint8_t game_buffer[256];

GAME uint32_t game_add_seven(uint32_t x) {
  return x + 7;
}

GAME uint32_t game_now_plus_one(uint32_t) {
  return API.now() + 1;
}

GAME uint32_t game_write(uint32_t address) {
  *reinterpret_cast<volatile uint32_t *>(address) = 0xBAD;
  return 1;
}

GAME uint32_t game_read(uint32_t address) {
  return *reinterpret_cast<volatile uint32_t *>(address) | 1;
}

GAME uint32_t game_svc_return(uint32_t address) {
  // asks for privilege outside the gate, then tries to use it
  __asm volatile("svc #255" ::: "memory");
  *reinterpret_cast<volatile uint32_t *>(address) = 0xBAD;
  return 1;
}

GAME uint32_t game_debug(uint32_t address) {
  API.debug(reinterpret_cast<const char *>(address));
  return 1;
}

GAME uint32_t game_erase(uint32_t) {
  API.erase_game(0);
  return 1;
}

GAME uint32_t game_launch(uint32_t) {
  game_buffer[0] = 'x';
  game_buffer[1] = 0;
  return API.launch(reinterpret_cast<const char *>(game_buffer)) ? 1 : 5;
}

GAME uint32_t game_loop(uint32_t) {
  while(true)
    __asm volatile("" ::: "memory");
}

GAME uint32_t game_recurse(uint32_t n) {
  volatile uint8_t pad[256];
  pad[0] = uint8_t(n);
  return n ? game_recurse(n - 1) + pad[0] : 0;
}

GAME uint32_t game_files(uint32_t fake_handle) {
  std::string name("save.dat");
  void *fh = API.open_file(name, 1);
  if(!fh)
    return 0;
  auto read = API.read_file(fh, 0, 4, reinterpret_cast<char *>(game_buffer));
  API.close_file(fh);
  if(fake_handle)
    API.read_file(reinterpret_cast<void *>(fake_handle), 0, 4, reinterpret_cast<char *>(game_buffer));
  return uint32_t(read);
}

GAME uint32_t game_read_into(uint32_t address) {
  // a real file handle, but the loader's memory as the buffer
  std::string name("save.dat");
  void *fh = API.open_file(name, 1);
  return uint32_t(API.read_file(fh, 0, 4, reinterpret_cast<char *>(address)));
}
