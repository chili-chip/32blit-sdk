# Debugging

## Debug on the desktop first

Most bugs show up in the desktop build too, and there you have a full debugger. Build with debug information:

```bash
cmake --preset linux          # the preset builds in Debug mode
cmake --build out/build/linux
gdb ./out/build/linux/game.elf
```

In VS Code with the CMake Tools extension, pick the `linux` preset and press ++f5++. See [Visual Studio Code](../setup/vscode.md).

## Printing messages

`debugf()` works like `printf()`:

```cpp
debugf("player at %d,%d\n", int(player.x), int(player.y));
```

| Build | Where the output goes |
| ----- | --------------------- |
| Desktop | the terminal the game was started from |
| Console | USB serial (the console's CDC serial port) and UART0 on GPIO 0 |

On Linux, the console's serial port is usually `/dev/ttyACM0`; open it with `screen /dev/ttyACM0 115200`, `minicom`, or `picocom`. On Windows it's a COM port.

`debug("text")` prints a plain `std::string`. Both cost time on the console, so don't print every frame in a build you're measuring.

## Debugging on the console

For bugs that only happen on hardware, you can debug the console over SWD with a Raspberry Pi Debug Probe (or a Pico running Picoprobe) and OpenOCD.

You need:

* a debug probe connected to the console's SWD pins (SWDIO, SWCLK and GND);
* Raspberry Pi's [OpenOCD fork](https://github.com/raspberrypi/openocd), which supports the RP2350;
* `gdb-multiarch` (or `arm-none-eabi-gdb`).

Debugging works on a **standalone** build (the template's `vgc` preset), which contains the whole firmware and your game in one `.elf`:

```bash
cmake --preset vgc
cmake --build out/build/vgc
```

### Flash over SWD

```bash
cd out/build/vgc
openocd -f interface/cmsis-dap.cfg -f target/rp2350.cfg \
  -c "adapter speed 5000; program game.elf verify reset exit"
```

The template has VS Code tasks for this: **Build Pico 2**, **Flash Pico 2 (SWD)**, and **Build + Flash Pico 2**.

### Step through code

The template's `.vscode/launch.json` has a **Pico 2 (OpenOCD)** configuration for the Cortex-Debug extension. It builds the `vgc` preset, flashes it, and stops at `main`, with the RP2350's registers available from the Pico SDK's SVD file. Set breakpoints in your game as usual.

Remember that a standalone build replaces ChiliChip OS; flash the [factory image](../console/flashing.md#setting-up-a-console) afterwards.

## Common problems

**The game crashes or freezes on the console but not on the desktop.**
:   Usually memory: a big local array overflowing the 32 KB stack, or the heap running out. See [Performance and memory](performance.md#memory-tips). A crash shows as a frozen screen; on a standalone build, the debugger stops in the HardFault handler, and the call stack shows where.

**The game runs slowly on the console.**
:   Look for `double` maths, allocations in the loop, and pixel-by-pixel drawing. See [Performance and memory](performance.md#cpu-tips).

**`read_save()` always returns false.**
:   The game's title changed (saves are stored by title), or the struct grew since the save was written: a save shorter than the struct doesn't load.

**The console says "Not a ChiliChip game".**
:   The `.blit` was built for another device. Check `PICO_PLATFORM=rp2350-arm-s`, and delete the build folder: an old build folder can keep a cached platform from an earlier configuration.

**`random()` is ambiguous.**
:   Write `blit::random()`. With `using namespace blit`, it clashes with the C library's `random()`.

**`std::min` doesn't compile on the console but does on the desktop.**
:   `uint32_t` is `unsigned long` on the console and `unsigned int` on Linux, so `std::min(score, 200u)` mixes types there. Write `std::min<uint32_t>(score, 200)`.

**Metadata errors when building for the console.**
:   Console builds need an 8×8 `icon` and a 128×96 `splash`. See [Metadata](metadata.md#fields).
