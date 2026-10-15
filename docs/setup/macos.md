# Building & Running on macOS

These instructions cover building 32blit on macOS.

## Prerequisites

You will need build tools and CMake. Assuming you have [homebrew](https://docs.brew.sh/Installation) installed:

``` shell
xcode-select --install
brew install cmake
```

### Python3

Before trying to install python3, it's worth checking if you already have it installed (and if so, which version), by jumping to 'Verifying install', below. If you do already have it installed, skip this section.

#### Installing python3

Installing `python3` can be done with homebrew with a simple `brew install python` which installs both `python3` and `pip3`.

####  Installing pip3 dependecies

before installing 32blit tools, some binary requirements are needed:

```
brew install libjpeg
brew install freetype
````

Then install the 32blit tools from the SDK's `tools/requirements.txt`. It installs the `32blit` command (which pulls in `construct`, `bitstring` and the other Python dependencies) and pins `bitstring<5`, because bitstring 5 removed `ConstBitStream`, which the tools still use.

Homebrew's Python refuses `pip install` outside a virtual environment ("externally-managed-environment", [PEP 668](https://peps.python.org/pep-0668/)), so create one first:

```
python3 -m venv ~/venvs/32blit
source ~/venvs/32blit/bin/activate
python3 -m pip install -r tools/requirements.txt
```

Run the commands from the root of your `32blit-sdk` checkout, and activate the venv (`source ~/venvs/32blit/bin/activate`) in every new terminal before building.

####  Verifying install

With the venv activated:

``` shell
python3 --version
```
(expected output `Python 3.x.y`, the version of Homebrew's Python)

``` shell
python3 -m pip --version
```
(expected output `pip x.y.z from /Users/<you>/venvs/32blit/lib/python3.x/site-packages/pip (python 3.x)` or similar; the path must be inside the venv)

and
``` shell
32blit version
```
(expected output: the installed 32blit tools version, `0.7.3` or newer)

<a name="gcc"/></a>
### Installing `gcc-arm-none-eabi`

Once this is done, you'll need to install `gcc-arm-none-eabi`. The easiest way to install this tool is via homebrew with the following source:

``` shell
brew tap ArmMbed/homebrew-formulae
brew install arm-none-eabi-gcc
```

Note:
If you do not want to/ are unable to use homebrew to do this, you should be able to find the manual install instruction in the `arm-none-eabi-gcc.rb` file in [this repository](https://github.com/ARMmbed/homebrew-formulae).

______

## Building & Running on the console

To build for the VGC Zero, install the console tools from [Install](../getting-started/install.md#console-tools) and follow [Run on the console](../getting-started/build-console.md). For the original 32blit, see [Building & Running On 32Blit](../hardware/32blit.md).

## Building & Running Locally

You'll need to install `SDL2`, `SDL2 Image` and `SDL2 Net`

``` shell
brew install sdl2 sdl2_image sdl2_net
```

You also need something to build. The [32blit-examples](https://github.com/32blit/32blit-examples) repository includes a series of demos showcasing various 32blit SDK features. These instructions will assume you're building those and have cloned or extracted that repository alongside 32blit-sdk:

```shell
git clone https://github.com/32blit/32blit-examples
```

Your directory tree should look something like:

- root_dir
    - 32blit-sdk
    - 32blit-examples

Create a build directory and configure the examples. The 32blit-sdk should be automatically detected:

```shell
mkdir build
cd build
cmake ..
```

Now to make any example, type:

```shell
make example-name
```

For example:

```shell
make raycaster
```

This will produce `examples/raycaster/raycaster` which you should run with:

```shell
./examples/raycaster/raycaster
```

### Build Everything

Alternatively you can build everything by just typing:

```shell
make
```

When the build completes you should be able to run any example.

## Troubleshooting

If you see `cannot create target because another target with the same name already exists` you've probably run `cmake ..` in the wrong directory (the project directory rather than the build directory), you should remove all but your project files and `cmake ..` again from the build directory.
