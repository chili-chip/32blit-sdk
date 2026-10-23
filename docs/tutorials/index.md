# Tutorials

These lessons build up the SDK one piece at a time. Each one comes with a small, complete program in [`docs/examples`](https://github.com/chili-chip/32blit-sdk/tree/master/docs/examples) that you can build and change. The last lesson puts everything together into a finished game.

| # | Lesson | You'll learn | Example |
| - | ------ | ------------ | ------- |
| 1 | [The game loop](game-loop.md) | `init`, `update` and `render`; screen modes; drawing shapes; time | `hello-world`, `shapes` |
| 2 | [Input](input.md) | reading buttons, presses and releases; moving a player | `input` |
| 3 | [Sprites and assets](sprites.md) | packing images with `assets.yml`; drawing, animating and transforming sprites | `sprites` |
| 4 | [Text](text.md) | fonts, alignment, measuring and wrapping text | `text` |
| 5 | [Sound](sound.md) | synth channels, envelopes, sound effects, music files | `audio` |
| 6 | [Saving progress](saving.md) | `read_save`, `write_save` and `on_quit` | `saves` |
| 7 | [Timers, tweens and particles](animation.md) | things that happen over time | `timers-tweens`, `particles` |
| 8 | [Tile maps](tilemaps.md) | building and scrolling a level | `tilemap` |
| 9 | [Make a complete game](complete-game.md) | structuring a whole game | `chili-catch` |

## Building the examples

All the examples build together. From the SDK's folder:

```bash
cmake -B build.examples -S docs/examples
cmake --build build.examples
./build.examples/hello-world/hello-world
```

For the console, add the usual options and install the `.blit` files:

```bash
cmake -B build.examples-vgc -S docs/examples \
  -DPICO_SDK_PATH=../pico-sdk \
  -DPICO_BOARD=chilichip_vgc \
  -DPICO_PLATFORM=rp2350-arm-s \
  -DCMAKE_TOOLCHAIN_FILE=$PWD/pico2.toolchain \
  -DBLIT_EXECUTABLE_PICO_BLIT=1
cmake --build build.examples-vgc
```

Then copy, for example, `build.examples-vgc/chili-catch/chili-catch.blit` onto the console's drive.

To start your own game from an example, copy its `.cpp` file over `game.cpp` in a project made from the [template](../getting-started/first-game.md). The examples that use sprites need `docs/examples/shared/sprites.png` and an `assets.yml` entry for it, as shown in [Sprites and assets](sprites.md).
