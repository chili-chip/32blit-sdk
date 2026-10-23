# Building & Running on Win32 (WSL and MinGW)

These instructions cover setting up Windows Subsystem for Linux so that you can cross-compile for 32Blit.

A basic knowledge of the Linux command-line, installing tools and compiling code from source is assumed.

## Setting Up

### Windows Subsystem for Linux (WSL)

To enable Windows Subsystem for Linux, you must access the _Turn Windows features on or off_ dialog. Find the entry for _Windows Subsystem for Linux_ and make sure it is enabled.

After that, proceed to the Microsoft Store to download Ubuntu for WSL.

### Installing requirements inside WSL

The following requirements enable cross-compile to 32Blit via ARM GCC:
(These are similar to the [Linux requirements](linux.md#prerequisites), see those for more details)

```
sudo apt install gcc gcc-arm-none-eabi unzip cmake make python3 python3-pip python3-setuptools python3-venv
```

Then install the 32blit tools from the SDK's `tools/requirements.txt`. It installs the `32blit` command (which pulls in `construct`, `bitstring` and the other Python dependencies) and pins `bitstring<5`, because bitstring 5 removed `ConstBitStream`, which the tools still use.

Current Ubuntu releases for WSL (23.04 and later, including 24.04) refuse `pip install` outside a virtual environment ("externally-managed-environment", [PEP 668](https://peps.python.org/pep-0668/)), so create one first:

```
python3 -m venv ~/venvs/32blit
source ~/venvs/32blit/bin/activate
python3 -m pip install -r tools/requirements.txt
```

Run the commands from the root of your `32blit-sdk` checkout, and activate the venv (`source ~/venvs/32blit/bin/activate`) in every new terminal before building.

If you are using the older Ubuntu 18.04-based WSL, you will need a newer toolchain than the one provided:
```shell
# This repo backports the toolchain from Ubuntu 19.10
sudo add-apt-repository ppa:daft-freak/arm-gcc
sudo apt update
```

## Building & Running on the console

To build for the VGC Zero, install the console tools from [Install](../getting-started/install.md#console-tools) and follow [Run on the console](../getting-started/build-console.md). For the original 32blit, see [Building & Running On 32Blit](../hardware/32blit.md).

## Building & Running Locally

You can use WSL on Windows to cross-compile your project (or any 32Blit example) into a Windows .exe for testing locally. This approach is included for completeness but not recommended, since MinGW binaries are statically linked and much larger than the Visual Studio output.

If you're more familiar with Visual Studio then you should [follow the instructions in Windows-VisualStudio.md](visual-studio.md)

You will need to install SDL2 and SDL2_image/_net for MinGW.

### Installing the MinGW compiler/tools

The following packages enable cross-compiling to Windows via MinGW:
(Assuming you have installed the tools for building to 32Blit above.)

```shell
sudo apt install gcc-mingw-w64 g++-mingw-w64
```

### Installing SDL2, SDL2_image and SDL2_net

This will install the SDL2 64bit mingw development headers and libraries into `/opt/local/x86_64-w64-mingw32/`.

Note: the `lib/cmake/SDL2/sdl2-config.cmake` shipped with these libraries expects them to be in `/opt/local`, if you change the install path you will have to modify this file.

First, make sure the `/opt/local/` directory exists:

```shell
sudo mkdir -p /opt/local/
```

#### SDL2

Grab and install the SDL2 mingw development package:

```shell
wget https://github.com/libsdl-org/SDL/releases/download/release-2.24.0/SDL2-devel-2.24.0-mingw.tar.gz 
tar xzf SDL2-devel-2.24.0-mingw.tar.gz
sudo cp -r SDL2-2.24.0/x86_64-w64-mingw32 /opt/local/
```

#### SDL2_image

Grab and install the SDL2_image mingw development package:

```shell
wget https://github.com/libsdl-org/SDL_image/releases/download/release-2.6.2/SDL2_image-devel-2.6.2-mingw.tar.gz
tar xzf SDL2_image-devel-2.6.2-mingw.tar.gz
sudo cp -r SDL2_image-2.6.2/x86_64-w64-mingw32 /opt/local/
```

#### SDL2_net

Grab and install the SDL2_net mingw development package:

```shell
wget https://github.com/libsdl-org/SDL_net/releases/download/release-2.2.0/SDL2_net-devel-2.2.0-mingw.tar.gz
tar xzf SDL2_net-devel-2.2.0-mingw.tar.gz
sudo cp -r SDL2_net-2.2.0/x86_64-w64-mingw32 /opt/local/
```

### Building

Finally, set up the 32Blit Makefile from the root of the repository with the following commands:

```shell
mkdir build.mingw
cd build.mingw
cmake .. -DCMAKE_TOOLCHAIN_FILE=../mingw.toolchain
```

#### Single Example

Now to make any example, type:

```shell
make example-name
```

For example:

```shell
make raycaster
```

This will produce `examples/raycaster/raycaster.exe` which you should run with:

```shell
./examples/raycaster/raycaster.exe
```

WSL will launch the example in Windows, using the required `SDL2.dll` that will have been copied into the build root.

Don't forget to include `SDL2.dll` this if you want to redistribute a game/example.

#### Build Everything

Alternatively you can build everything by just typing:

```shell
make
```

When the build completes you should be able to run any example.

### Troubleshooting

If you see `cannot create target because another target with the same name already exists` you've probably run `cmake ..` in the wrong directory (the project directory rather than the build directory), you should remove all but your project files and `cmake ..` again from the build directory.
