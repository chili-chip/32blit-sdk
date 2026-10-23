# Storage, saves and memory

## Flash layout

The VGC Zero has 16 MB of flash, split three ways:

| Region | Contents |
| ------ | -------- |
| 0–256 KB | the loader (ChiliChip OS firmware) |
| 256 KB–12 MB | installed `.blit` games, in 64 KB blocks; the home screen is the first |
| 12–16 MB | a FAT file system, shown on your computer as the **CHILICHIP** USB drive |

Games run straight from flash (execute in place). The RP2350's address translation maps whichever game is running to the same address, so every `.blit` is built for one fixed location and can be installed anywhere.

## The USB drive

The 4 MB drive holds everything that changes:

```
CHILICHIP/
├── .system/
│   └── settings.bin     # brightness, volume, screen sleep
├── .blit/
│   └── <game title>/
│       ├── save0        # write_save(data, 0)
│       └── save1        # write_save(data, 1)
└── game.blit            # copied here to install, then removed
```

Your game can also read and write its own files on the drive with the [Files API](../api/files.md).

## Saves

`write_save()` writes slot *n* to `.blit/<title>/save<n>`, where `<title>` is the `title` from your `metadata.yml`. Because saves live on the drive rather than with the game:

* reinstalling or updating a game keeps its saves;
* players can back up a game's saves by copying its folder;
* changing your game's title loses access to the old saves, so keep the title stable.

When a game is deleted from the home screen and has saves, the console asks whether to delete them too.

The console has a hard power switch, and switching it off can't be caught. Save at checkpoints as well as in `on_quit`. See [Saving progress](../tutorials/saving.md).

## RAM

The RP2350 has 520 KB of RAM. The loader keeps the first 64 KB; the running game gets the rest:

| Region | Contents |
| ------ | -------- |
| 0–64 KB | loader data, its heap, and the 32 KB core 0 stack that games run on |
| 64–512 KB | the running game: its code that must run from RAM, data, heap, and the framebuffer |

So a game has **448 KB** for everything, including its framebuffer: 32 KB in `hires` mode, 8 KB in `lores`. Your stack is 32 KB, the same as a standalone build.

A `.blit` built against an SDK from before the loader's RAM grew to 64 KB expects games to start at 32 KB, and must be rebuilt.

See [Performance and memory](../guides/performance.md) for how to keep within these limits.
