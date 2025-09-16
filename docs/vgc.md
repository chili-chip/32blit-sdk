# Building 32Blit SDK for ChiliChip VGC Zero

This guide explains how to build 32Blit SDK projects for **ChiliChip VGC Zero**, a **RP2350-based** video game console.

---

## Prerequisites

Install dependencies (Ubuntu / WSL):

```bash
sudo apt install git gcc g++ gcc-arm-none-eabi cmake make \
python3 python3-pip python3-setuptools \
libsdl2-dev libsdl2-image-dev libsdl2-net-dev unzip
```

Install the 32Blit Python tools:

```bash
python3 -m pip install --user 32blit
```

If pip installs to a directory not on PATH:

```bash
export PATH=$PATH:~/.local/bin
```

---

## Directory Structure

Recommended project layout:

```
project_root/
├── 32blit-sdk/
├── pico-sdk/
├── pico-extras/
└── your-project/
```

---

## Configuring the Build for ChiliChip VGC Zero

Create a build directory:

```bash
cd your-project
mkdir build.vgc
cd build.vgc
```

Run `cmake` with the board and platform specified:

```bash
cmake .. \
  -D32BLIT_DIR=../../32blit-sdk \
  -DPICO_SDK_PATH=../../pico-sdk \
  -DPICO_BOARD=chilichip_vgc \
  -DPICO_PLATFORM=rp2350-arm-s \
  -DCMAKE_TOOLCHAIN_FILE=../../32blit-sdk/pico2.toolchain
```

> Adjust paths if your SDKs are located elsewhere relative to your project directory.

---

## Building the Project

```bash
make
```

This produces a `.uf2` file for your game.

---

## Copying to ChiliChip VGC Zero

1. Connect the console via USB.
2. Power it off, then hold the top **X button** and press **Power**.
3. The device should appear as `RPI-RP2` on Linux (`/media/<username>/RPI-RP2`).
4. Copy your `.uf2` file:

The console will automatically reboot into your game.

---

## ChiliChip VGC Zero Board Details

| Feature  | VGC Zero (RP2350)             |
| -------- | ----------------------------- |
| CPU      | RP2350, 150 MHz (dual-core)   |
| RAM      | 520KB + 16MB                  |
| Controls | 8 buttons + encoder           |
| Screen   | 128x128 st7735s               |
| Sound    | Mono speaker                  |
| Storage  | 16MB XiP QSPI                 |

The RP2350 in VGC Zero is significantly faster than the RP2040, allowing higher framerates, more complex logic, and improved audio support.

## References

* [32blit SDK GitHub](https://github.com/32blit/32blit-sdk)
* [32blit Examples](https://github.com/32blit/32blit-examples)
* [Pico SDK Getting Started Guide](https://datasheets.raspberrypi.org/pico/getting-started-with-pico.pdf)
