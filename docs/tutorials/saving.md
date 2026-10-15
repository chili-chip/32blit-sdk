# 6. Saving progress

![Saves](../assets/screenshots/saves.png){ .screenshot align=right }

Players expect a game to remember their best score, their unlocked levels and their settings. The SDK saves a block of data per **slot**, and on the console calls your game just before the player quits so it can save.

```cpp title="saves.cpp"
--8<-- "saves/saves.cpp"
```

## Save data is a struct

Put everything you want to keep in one plain struct of numbers, booleans and fixed-size arrays:

```cpp
struct SaveData {
  uint32_t version = 1;
  uint32_t best = 0;
  uint32_t games_played = 0;
};
```

The SDK copies the struct's bytes to the file as they are, so don't put pointers, `std::string` or `std::vector` in it: they point at memory that won't be there next time. Use `char name[16]` instead of a string.

## Loading

```cpp
if(!read_save(save) || save.version != 1)
  save = SaveData();
```

`read_save()` returns `false` if there is no save yet, or if the file is shorter than the struct, so a first run starts from the defaults. A file that's *longer* still loads, which is why the `version` check matters. Checking a `version` field as well lets a later version of your game spot (and convert) saves from an older layout.

## Writing

```cpp
write_save(save);
```

Writes the whole struct to slot 0. Saving takes a few milliseconds on the console because it writes to flash, so do it at natural moments, not every update.

## Saving when the player quits

On the VGC Zero, the player leaves a game from the system menu or by holding HOME. Set `on_quit` to a function and the OS calls it first:

```cpp
void save_now() {
  write_save(save);
}

void init() {
  ...
  on_quit = save_now;
}
```

On the desktop, `on_quit` is called when the window closes.

!!! warning "Also save at checkpoints"
    The console has a hard power switch, and switching it off can't be caught: `on_quit` isn't called. Save when something worth keeping happens, like the end of a level or a new high score, as well as on quit. The example saves at the end of every round.

`on_quit` was added in API 0.5, so games built with this SDK need a console loader from API 0.5 or later; an older console shows "Needs a system update".

## Slots

Every save function takes an optional slot number, so a game can keep several independent saves:

```cpp
write_save(settings, 0);   // slot 0: options
write_save(progress, 1);   // slot 1: story progress
read_save(progress, 1);
remove_save(1);            // delete slot 1
```

Each slot is its own file, and can be a different struct.

## Where saves go

On the console, slot *n* is the file `.blit/<title>/save<n>` on the USB drive, where `<title>` is your game's title from `metadata.yml`. Saves stay there when the game is updated or reinstalled, and players can back them up by copying the folder. If you change the title, the game will look in a new folder and start fresh.

On the desktop, saves go to a per-user folder named after the game's author and title, for example `~/.local/share/<author>/<title>/` on Linux.

See [Storage, saves and memory](../console/storage.md) for the details.

## Try it

* Add a "longest round without missing" statistic to the save.
* Bump `version` to 2 with a new field, and convert version 1 saves instead of throwing them away.

Next: [timers, tweens and particles](animation.md).
