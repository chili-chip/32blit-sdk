# Flashing and the loader

Most of the time you only copy `.blit` games onto a console. This page covers the lower-level jobs: setting up a new console, updating ChiliChip OS, and building it yourself.

## BOOTSEL mode

The RP2350 has a built-in bootloader that shows up as a USB drive and accepts `.uf2` files. To enter it:

1. Connect the console to your computer.
2. Switch it off, then hold the top **X** button and switch it on.
3. A drive named `RP2350` appears (on Linux usually at `/media/$USER/RP2350`).

Copy a `.uf2` onto that drive and the console writes it to flash and restarts.

## Setting up a console

`vgc-os-factory.uf2` holds the loader with the home screen already installed, so one copy sets up a new or wiped console:

```bash
cp vgc-os-factory.uf2 /media/$USER/RP2350/
```

The console restarts into the home screen, ready for games to be [copied onto its drive](chilichip-os.md#installing-games).

The image writes the loader to flash 0–256 KB and the home screen at 256 KB, and leaves the rest of flash alone: other installed games and the USB drive (saves and settings) are kept. A game that was installed at 256 KB is overwritten and needs installing again.

## Updating the OS

* **The home screen** is a `.blit`: copy a new `vgc-os.blit` onto the console's drive like a game.
* **The loader** is a `.uf2`: copy a new `blit-loader.uf2` in BOOTSEL mode. This keeps installed games, saves and settings. Do this when a game reports "Needs a system update".

## Building the OS

Build the loader, the home screen and the factory image from the SDK itself:

```bash
cd 32blit-sdk
mkdir build.vgc-blit
cd build.vgc-blit
cmake .. \
  -DPICO_SDK_PATH=../../pico-sdk \
  -DPICO_BOARD=chilichip_vgc \
  -DPICO_PLATFORM=rp2350-arm-s \
  -DCMAKE_TOOLCHAIN_FILE=../pico2.toolchain \
  -DBLIT_EXECUTABLE_PICO_BLIT=1
make
```

| Output | What it is |
| ------ | ---------- |
| `32blit-pico/loader/blit-loader.uf2` | the loader (system firmware) |
| `vgc-os/vgc-os.blit` | the home screen |
| `vgc-os/vgc-os-factory.uf2` | the loader with the home screen pre-installed |
| `launcher/launcher.blit` | the stock 32blit launcher, as an alternative home screen |

The CI's "VGC-blit" build produces the same files.

`tools/factory-uf2.py` makes the factory image by adding `.blit` files at fixed flash offsets (in KB) to the loader's `.uf2`, numbered as one image:

```bash
python3 tools/factory-uf2.py blit-loader.uf2 vgc-os.blit@256 -o vgc-os-factory.uf2
```

The same tool can preload games, for example for a batch of consoles: add a `game.blit@<offset>` argument per game. Offsets must be multiples of 64 and must not overlap: each game starts after the end of the previous file, rounded up to the next 64 KB.

## Installing over USB serial

With the console running the loader and connected over USB, the 32blit tool installs `.blit` files without going through the drive:

```bash
32blit install vgc-os.blit
32blit install game.blit
```

### Serial commands

Tools talk to the loader over the USB serial port. Each command starts with `32BL` and a four-letter name. Strings end with a NUL byte and numbers are little-endian.

The loader only acts on a command once the byte after its name arrives. Commands with no arguments (`__LS`, `VERS`) are sent with a trailing NUL, or the loader waits.

| Command | Sends | Loader replies | Use |
| ------- | ----- | -------------- | --- |
| `32BLPROG` | file name, size, data | `32BL__OK` and the flash block (uint16) | install a `.blit` |
| `32BLSAVE` | path, size, data | `32BL__OK` | write a file to the drive |
| `32BL__LS` | NUL | per game: offset, size, metadata; then `0xFFFFFFFF` | list installed games |
| `32BLLNCH` | path | nothing | launch a game or file |
| `32BLERSE` | flash offset (uint32) | nothing | delete a game |
| `32BLVERS` | NUL | `32BLVERS`, the loader version, API major and minor (uint16 each) | check the firmware version |

The loader version is the release tag it was built from, for example `v1.0.1`, or the `git describe` output for a local build. A loader older than this command prints `got: 32BLVERS` and a line break instead. Tools read that line, so it does not end up in front of the next reply, and treat the loader as out of date.

```python
import serial, struct

with serial.Serial("/dev/ttyACM0", timeout=2) as port:
    port.write(b"32BLVERS\0")
    if port.read(8) != b"32BLVERS":
        port.readline()  # older loader: "got: 32BLVERS"
        raise SystemExit("loader is older than 32BLVERS")
    version = b""
    while (c := port.read(1)) not in (b"\0", b""):
        version += c
    api_major, api_minor = struct.unpack("<HH", port.read(4))
    print(version.decode(), f"API {api_major}.{api_minor}")
```

## Build options

These are set for the console in `32blit-pico/board/chilichip_vgc/config.h` and `config.cmake`. You only need them when changing the OS itself.

| Setting | Effect |
| ------- | ------ |
| `SYSTEM_MENU` | a short HOME press opens the system menu (loader only) |
| `AUTO_INSTALL` | `.blit` files on the drive install after eject; the drive is read-only while a game runs (loader only) |
| `STORAGE_VOLUME_LABEL` | the drive label, `CHILICHIP`; applied at boot, so older drives are relabelled |
| `USB_VENDOR_STR`, `USB_PRODUCT_STR` | the USB device name, "ChiliChip Console" |
| `BATTERY_ADC_PIN`, `BATTERY_DIVIDER`, `BATTERY_OFFSET_MV` | battery sensing on GPIO 29 through a ÷3 divider, plus the 0.3 V diode drop in front of it |
| `LCD_MAX_CLOCK` | the display's SPI clock cap, 20 MHz |
| `BLIT_ENABLE_CORE1` | mix audio on the second core |
| `BLIT_LOADER_RAM_KB` | RAM reserved for the loader, 64 KB |

The install logic is `install_blit_file()` in `32blit-pico/blit_launch.cpp`, the serial commands are in `32blit-pico/usb.cpp`, the drive watcher is `32blit-pico/installer.cpp`, and the system menu is `32blit-pico/system_menu.cpp`.
