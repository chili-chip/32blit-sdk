# Game sandbox

!!! warning "Not on by default yet"
    The sandbox is built only with `-DBLIT_SANDBOX=ON` and has not been tested on the console yet
    ([#91](https://github.com/chili-chip/32blit-sdk/issues/91)). Games built with the SDK need no change for it.

A `.blit` game is native code. Without a sandbox the loader calls it directly: it runs privileged and can
read or write all flash (other games, saves, the loader), the loader's memory and every peripheral.
With the sandbox the loader runs every call into the game (`init`, `tick`, `render`, `on_quit`, the
multiplayer callback) like this:

- **unprivileged**, on its own stack, with the Cortex-M33 **MPU** allowing only the game's own flash,
  its own RAM, its stack, and the small shared area the API uses (buttons, audio channels);
- the game reaches the loader **only through the API**: every APIConst slot it sees is a 4-byte
  trampoline `svc #n; bx lr`, and the loader checks every pointer before making the call;
- a game that touches anything else, overflows its stack or hangs when you hold HOME is **stopped**
  and the console goes back to the home screen, instead of crashing.

Why the MPU and not TrustZone-M: see [#94](https://github.com/chili-chip/32blit-sdk/issues/94).

## What a sandboxed game can't do

| | |
|---|---|
| Touch hardware directly (registers, `time_us_32()`, the SIO, DMA, PIO) | stopped: use the API |
| Write or read another game, the loader's RAM or the system registers | stopped |
| Pass the API a pointer that is not its own memory, or a made-up file handle | stopped |
| `erase_game` | stopped |
| `launch`, `list_installed_games`, `set_system_settings`, `get_type_handler_metadata`, `list_files`, JPEG decoding, I2C, raw USB | refused: returns 0 / `false`, the game goes on |
| Audio `wave_buffer_callback` (WAV and MP3 streaming) | not called: it would run privileged on core 1 |
| Multiplayer messages over 512 bytes | dropped |
| More than 16 open files | `open_file` returns `nullptr` |

The game's stack is 24 KB (the loader keeps 8 KB of its 32 KB stack area). The home launcher
(category `launcher`) is trusted and runs privileged; signed builds will decide what is trusted later.

## How it works

`32blit-pico/sandbox_gate.S` holds the trampolines, `sandbox_enter` (drop privilege, switch to the process
stack, call the game), `sandbox_return` (the game returns there; its `svc #255` gives privilege back only at
that address) and the SVC and fault handlers. `32blit-pico/sandbox.cpp` sets up the MPU and checks each API call.

| MPU region | Memory | Game access |
|---|---|---|
| 0 | loader flash (trampolines, APIConst) | read, execute |
| 1 | the game's image | read, execute |
| 2 | its installed copy, if address-translated (metadata) | read |
| 3 | APIData, audio channels, copies the API returns | read, write |
| 4 | the game's stack | read, write |
| 5 | the game's RAM (`0x20010000` to `0x20080000`) | read, write, execute |

## Build and test

```
cmake .. -DPICO_BOARD=chilichip_vgc -DPICO_PLATFORM=rp2350-arm-s -DCMAKE_TOOLCHAIN_FILE=../pico2.toolchain -DBLIT_SANDBOX=ON
make blit-loader              # the loader with the sandbox
make run-sandbox-test         # sandbox.cpp on QEMU's Cortex-M33 (needs qemu-system-arm)
```

The QEMU test (`32blit-pico/sandbox_test/`) runs the real gate and checks on an emulated Cortex-M33 with a
small fake game: writes to the loader's RAM, `svc #255` outside the gate, bad pointers, `erase_game`, an
endless loop, a stack overflow and file handles. CI runs it in the "VGC (sandbox)" job. Only the console
can test the real loader, display, audio and USB.
