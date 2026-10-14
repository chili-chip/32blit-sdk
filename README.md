# ChiliChip Game SDK

This is a fork of the [32blit SDK](https://github.com/32blit/32blit-sdk), adapted to build games for the **ChiliChip VGC Zero** console. The VGC Zero is a handheld built on the RP2350 with a 128x128 SSD1351 OLED, PWM audio, 16 MB of flash and A/B/X/Y, MENU, HOME and d-pad buttons.

Everything from upstream 32blit still works: the same engine and API, the SDL build for Linux, Windows, macOS and the browser, and the 32blit and other RP2040/RP2350 targets. We merge upstream changes regularly, and all credit for the engine, tools and the original Pico port goes to the [32blit team and contributors](https://github.com/32blit/32blit-sdk/graphs/contributors).

To start a new VGC Zero game, use the [ChiliChip game template](https://github.com/chili-chip/game-template). Build instructions for the console are in [docs/vgc.md](docs/vgc.md).

## What this fork adds

### VGC Zero hardware support

* A `chilichip_vgc` board for the 32blit Pico port, with the correct 16 MB flash layout (games from 256 KB to 12 MB, the storage drive from 12 to 16 MB).
* A new SSD1351 OLED display driver, tuned to remove tearing, flicker and rolling scan lines, with a 20 MHz SPI clock so the screen can run well above 30 FPS.
* Audio through hardware PWM, mixed on core 1 from RAM so it no longer collides with the display or stalls on flash.
* A 32 KB stack on core 0, so larger C++ games no longer overflow it.
* A binary LED driver, debounced GPIO buttons and the console's button pin mapping.

### VGC OS: a console operating system

Instead of flashing one game at a time, the VGC Zero runs the 32blit loader and installs games as `.blit` files.

* **Loader and `.blit` games.** The loader boots from the first 256 KB of flash and runs installed games, with HOME returning to the home screen. The drive is rescanned after the PC writes to it, so new games appear without a restart.
* **Battery, settings and the HOME menu.** Battery level, voltage and USB state come from the ADC. Volume and brightness are saved once and applied in every game. A short HOME press opens a system menu to resume, change volume or brightness, check the battery or quit. These are exposed to games through API 0.3, with a `system-test` utility to try them.
* **Install by USB drive.** The console shows up as a `CHILICHIP` drive. `.blit` files copied onto it install on the home screen after you eject, with a progress bar. Newer builds replace older ones and keep their saves, and wrong-device, damaged or too-large files get a plain error.
* **Home screen.** The `vgc-os` app ("ChiliChip OS") is a chili-themed carousel of game covers with a settings screen, built on the system info call added in API 0.4.

### Tooling

* CI builds the VGC Zero firmware and `.blit` targets alongside the upstream ones.
* CODEOWNERS and Dependabot for this repository.

---

The rest of this README is the upstream 32blit documentation, which still applies to this fork.

# 32blit SDK

This is the software developer kit for 32blit. It includes the 32blit game engine, plus tooling and instructions to compile your 32blit projects on Linux, Windows and macOS.

Finished 32blit games can be released simultaneously for Linux, Windows, macOS and the 32blit using the GitHub actions in the 32blit boilerplate - https://github.com/32blit/32blit-boilerplate/

# You Will Need

Some experience writing/compiling C/C++ software is required.

OS specific docs, these cover setup and command-line builds:

* [Linux](docs/Linux.md)
* [Linux - ChromeOS](docs/ChromeOS.md)
* [Windows](docs/Windows.md)
* [Windows - WSL (Advanced)](docs/Windows-WSL.md)
* [macOS](docs/macOS.md)

If you want to run in a browser, refer to the [Emscripten docs](docs/Emscripten.md)

If you want to run on device, refer to the [VGC Zero docs](docs/vgc.md) for the ChiliChip console, or the [32blit docs](docs/32blit.md) and [Pico docs](docs/pico.md) for other hardware.

How to set up your editor:

* [Visual Studio Code](docs/VSCode.md)

# Overview of what is inside?

## 32blit

The `32blit` directory contains the API/engine for developing your game. This engine contains all the graphics, sound and game logic functionality you'll need for building a 32blit game.

## 32blit-sdl

The `32blit-sdl` directory contains an SDL2 HAL for 32blit compatible with Linux, macOS, Windows and Emscripten. You can use it to run your 32blit projects on your computer and test/debug them a little quicker.

You should be careful relying upon it, however, since `32blit-sdl` is *not an emulator* you may run into memory or performance problems when deploying your game to a 32blit console.

## 32blit-pico

The `32blit-pico` directory contains the HAL for RP2040 and RP2350 boards, including the ChiliChip VGC Zero (`chilichip_vgc`), plus the loader and VGC OS system services. See [docs/vgc.md](docs/vgc.md).

## vgc-os

The `vgc-os` directory contains the VGC Zero home screen app, installed on the console as a `.blit` launcher.

## 32blit-stm32

The `32blit-stm32` directory contains the STM32 HAL for 32blit, compatible with the STM32H750. Once you're ready to get your project running on a 32blit console you will need to ensure you have the `gcc-arm-none-eabi` toolchain installed and then follow the instructions below for [Building & Running On 32Blit](docs/32blit.md)

## Examples / Projects

The [32blit-examples](https://github.com/32blit/32blit-examples) repository contains example projects, these can be built into both SDL or STM32 binaries and cover a range of techniques from simple concepts to complete games.

Refer to the OS/platform specific documentation files in the `docs/` folder for instructions on how to compile and run these examples.

To get started writing your own game, you should use the 32blit boilerplate - https://github.com/32blit/32blit-boilerplate/

## Help & Support

Join the 32blit community on:

* Discord (Chat) - https://discord.gg/7qM9ftC
* Discourse (Forum) - https://forums.pimoroni.com/c/32blit/21

## Troubleshooting

If you see `cannot create target because another target with the same name already exists` you've probably run `cmake ..` in the wrong directory (the project directory rather than the build directory), you should remove all but your project files and `cmake ..` again from the build directory.

If you are getting complaints on WSL / Linux about python modules missing, you may have accidentally installed them with sudo. When using pip3 to install modules do not use sudo, this will make sure that modules are installed for the current user.

