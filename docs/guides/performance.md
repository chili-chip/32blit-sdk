# Performance and memory

The VGC Zero is fast for a handheld of its size, but it's a microcontroller, not a PC. A game that runs smoothly on your computer can still drop frames or run out of memory on the console. This guide covers the limits and how to stay inside them.

## The budget

| Resource | VGC Zero |
| -------- | -------- |
| CPU | one Cortex-M33 core at 250 MHz for your game (the other core mixes audio) |
| RAM | 448 KB for everything: code that runs from RAM, data, heap, stack and the framebuffer |
| Stack | 32 KB |
| Flash | 11.75 MB for all installed games together; code runs in place through a 16 KB cache |
| Frame | sending a full frame to the screen takes about 13 ms, by DMA in the background |

`update()` runs 100 times a second. If your `update()` takes longer than 10 ms, the game slows down: the desktop build shows `[SLOW]` in the title bar in the same situation, so watch for it.

## Measure first

Time the parts of your game with `now_us()`:

```cpp
uint32_t start = now_us();
draw_level();
uint32_t took = us_diff(start, now_us());
debugf("level: %lu us\n", (unsigned long)took);
```

For ongoing measurements, the [Profiler](../api/profiler.md) keeps running averages and can draw them over your game. Measure on the console: desktop timings say little about it.

## CPU tips

* **Use `float`, never `double`.** The M33 has a hardware single-precision FPU; `double` maths runs in software and is many times slower. Write constants as `1.5f`, not `1.5`, and use `sinf()`, `sqrtf()` and friends. The template compiles with `-Wdouble-promotion` to catch accidental doubles.
* **Prefer integers for positions on screen.** `Point` and `Rect` are integers; keep floats for physics.
* **Draw less.** Clearing the screen and redrawing everything is simplest, but a 128×128 `clear()` plus a full-screen background blit is already 32 K pixel writes. Draw a background with `rectangle()` or spans where you can, rather than pixel by pixel.
* **Spans beat pixels.** `h_span()` and `v_span()` are much faster than loops of `pixel()`.
* **Put hot functions in RAM.** Code runs from flash through a small cache. A function that runs for every pixel or every particle can be moved to RAM:

    ```cpp
    void blit_fast_code(mix_row)(uint16_t *row, int count) {
      ...
    }
    ```

    `blit_fast_code(name)` places the function in RAM on the console (and does nothing on the desktop). RAM is precious, so use it only for small, hot functions that measurements show are slow.

* **Avoid allocations in the game loop.** `new`, `std::vector::push_back` growth and `std::string` building all allocate. Reserve vectors in `init()`, and build strings for display only when they change.

## Memory tips

* **Use `lores` if your game suits it.** The 64×64 framebuffer is 8 KB instead of 32 KB.
* **Mind your spritesheets.** `Surface::load()` copies an image into RAM at one byte per pixel: a 128×128 sheet is 16 KB. `Surface::load_read_only()` uses an unpacked image straight from flash instead.
* **Keep big constant data `const`.** Tables, level data and text marked `const` stay in flash; non-const arrays are copied to RAM at start-up.
* **Watch the stack.** Large local arrays live on the 32 KB stack. Make big buffers global or allocate them.
* **Check what you've used.** The build's `.elf` can be inspected with `arm-none-eabi-size game.elf`, and the linker fails with a "region RAM overflowed" error when data doesn't fit.

## Frame rate

Your game draws a frame, then the SDK sends it to the screen while you start the next one. A full 128×128 frame takes about 13 ms to send, so the screen tops out a little above 70 frames a second; most games aim for a steady 30–60. Because `update()` runs at a fixed rate regardless, a lower frame rate makes the game choppier but never slower.

## Sound

Audio is mixed on the second core, from RAM, so it doesn't cost your game time. MP3 decoding is the exception: the decoder runs in `MP3Stream::update()` on your core. Prefer WAV or the synthesiser when time is tight.
