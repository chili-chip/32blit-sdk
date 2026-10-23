# Install the tools

You need two sets of tools: the **desktop tools** to build and run games on your computer, and the **console tools** to build them for the VGC Zero. Install the desktop tools first; you can add the console tools later.

## Folder layout

Keep the SDKs side by side in one folder, next to your games. The template's CMake presets and the commands in these docs assume this layout:

```
projects/
├── 32blit-sdk/      # this SDK
├── pico-sdk/        # Raspberry Pi Pico SDK (console builds only)
├── pico-extras/     # Pico extras (console builds only)
└── my-game/         # your game, made from the template
```

Clone the SDK:

```bash
mkdir projects && cd projects
git clone https://github.com/chili-chip/32blit-sdk
```

## Desktop tools

=== "Ubuntu / Debian / WSL"

    ```bash
    sudo apt install git gcc g++ cmake make \
      python3 python3-pip python3-setuptools python3-venv \
      libsdl2-dev libsdl2-image-dev libsdl2-net-dev unzip
    ```

    On Windows, install WSL first as described in [Windows (WSL)](../setup/windows-wsl.md), then run these commands inside it.

=== "macOS"

    Install the Xcode command line tools (`xcode-select --install`) and [Homebrew](https://brew.sh), then:

    ```bash
    brew install cmake sdl2 sdl2_image sdl2_net python libjpeg freetype
    ```

    See [macOS](../setup/macos.md) for details.

=== "Windows (Visual Studio)"

    Install Visual Studio 2019 or later with the "Desktop development with C++" workload, and Python 3. Visual Studio's CMake support builds the desktop version; see [Visual Studio](../setup/visual-studio.md). For console builds on Windows, use WSL.

### The 32blit tools

The `32blit` command line tool packs images, maps and fonts into your game and writes its metadata. CMake runs it for you, but it has to be installed. Install it from the SDK's requirements file, which pins the versions the SDK needs:

```bash
cd 32blit-sdk
python3 -m venv ~/venvs/32blit
source ~/venvs/32blit/bin/activate
python3 -m pip install -r tools/requirements.txt
```

Activate the virtual environment (`source ~/venvs/32blit/bin/activate`) in every new terminal before you build. Check it worked with `32blit --help`.

!!! note "Why a virtual environment?"
    Current Debian, Ubuntu and Homebrew Python refuse `pip install` outside a virtual environment ("externally-managed-environment", [PEP 668](https://peps.python.org/pep-0668/)). The requirements file also pins `bitstring<5`: bitstring 5 removed `ConstBitStream`, which the tools still use, so an unpinned `pip install 32blit` gives you tools that fail.

## Console tools

To build for the VGC Zero you also need the Arm cross compiler and the Raspberry Pi Pico SDK.

=== "Ubuntu / Debian / WSL"

    ```bash
    sudo apt install gcc-arm-none-eabi libnewlib-arm-none-eabi libstdc++-arm-none-eabi-newlib
    ```

=== "macOS"

    ```bash
    brew install --cask gcc-arm-embedded
    ```

    Or download the "AArch32 bare-metal target (arm-none-eabi)" toolchain from [Arm](https://developer.arm.com/downloads/-/arm-gnu-toolchain-downloads) and add its `bin` folder to your `PATH`.

Then clone ChiliChip's copies of the Pico SDK and Pico extras next to `32blit-sdk`. These are the versions the console is built and tested with:

```bash
cd projects
git clone --recurse-submodules https://github.com/chili-chip/pico-sdk
git clone https://github.com/chili-chip/pico-extras
```

The game template's CI uses the compiler from Ubuntu 22.04 (GCC 10.3); newer releases of the Arm GNU toolchain work too. If CMake can't find the compiler, check that `arm-none-eabi-gcc --version` runs.

## Editor

Any editor works. [Visual Studio Code](../setup/vscode.md) with the CMake Tools extension is a good fit: the game template includes presets for every target and a launch configuration for debugging on the console over SWD (see [Debugging](../guides/debugging.md)).

Next: [create your first game](first-game.md).
