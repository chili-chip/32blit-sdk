# Saves

`engine/save.hpp`, `engine/engine.hpp`

Saves are numbered **slots**, each holding one block of bytes, usually a struct. See the [saving tutorial](../tutorials/saving.md).

## Functions

```cpp
bool read_save(char *data, uint32_t length, int slot = 0);
void write_save(const char *data, uint32_t length, int slot = 0);
bool remove_save(int slot = 0);

template<class T> bool read_save(T &data, int slot = 0);
template<class T> void write_save(const T &data, int slot = 0);
```

`read_save(data, slot)`
:   Reads slot `slot` into `data`. Returns `true` if the save exists and holds at least `sizeof(data)` bytes; otherwise returns `false`, and `data` may be partly overwritten, so reset it to defaults.

`write_save(data, slot)`
:   Writes `data` to the slot, replacing what was there.

`remove_save(slot)`
:   Deletes the slot. Returns `true` if it was deleted.

`slot`
:   Any number. Each slot is a separate file and can hold a different type.

The template versions take any type and use `sizeof(T)`. The type must be [trivially copyable](https://en.cppreference.com/w/cpp/types/is_trivially_copyable): numbers, `bool`, enums, fixed-size arrays and structs of those. No pointers, `std::string` or containers.

```cpp
struct Progress {
  uint32_t version = 1;
  uint8_t level = 1;
  bool unlocked[10] = {};
  char name[12] = "PLAYER";
};

Progress progress;

if(!read_save(progress, 1) || progress.version != 1)
  progress = Progress();
```

## Saving when the player quits

```cpp
extern void (*on_quit)();
```

Set `on_quit` to a function that saves, and the SDK calls it when the player quits: from the system menu or by holding HOME on the VGC Zero, or by closing the window on the desktop. **API 0.5+**. See [`on_quit`](engine.md#on_quit) for the details, and save at checkpoints too: switching the console off isn't caught.

## Where saves are stored

| Platform | Slot *n* of a game titled *Title* |
| -------- | --------------------------------- |
| VGC Zero | `.blit/Title/save<n>` on the CHILICHIP USB drive |
| Linux, macOS, Windows | `save<n>` in SDL's per-user data folder for the game's author and title, e.g. `~/.local/share/<author>/<title>/` |
| Web | the browser's IndexedDB storage for the page |

On the VGC Zero saves are kept when a game is updated, and the player is asked whether to delete them when the game is deleted. Writing a save writes to flash and takes a few milliseconds.
