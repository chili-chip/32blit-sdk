# Building & Running For Web (Emscripten)

Building in Emscripten - asm.js/WebAssembly - works on Linux and WSL.

You will need to install the emscripten compiler. See https://emscripten.org/docs/getting_started/downloads.html for complete instructions, but generally you should just browse to your desired directory and:

```shell
git clone https://github.com/emscripten-core/emsdk.git
cd emsdk
./emsdk install latest
./emsdk activate latest
source ./emsdk_env.sh
```

You should now have commands like `emcc` in your path, and an `EMSDK` environment variable set ready to build your project.

In your project directory create a new build directory, enter it and configure your project:

```shell
mkdir build.em
cd build.em
emcmake cmake .. -D32BLIT_DIR="/path/to/32blit/repo"
```

:warning: Make sure to include the `-D32BLIT_DIR="/path/to/32blit/repo"` parameter to the `cmake` command (even when building the SDK examples). You must specify an absolute path here when building with Emscripten.

Once CMake has finished configuring your project you can build it:

```shell
make
python3 -m http.server
```

Finally, open the URL given by Python's HTTP server in your browser and open your project's .html file.

## Audio Mute Button

The web player includes a mute button that allows users to toggle audio on and off. The mute functionality is exposed through a JavaScript function `blit_set_audio_muted(muted)` that can be called with `1` to mute or `0` to unmute.

When muted, audio processing continues internally to maintain proper audio state, but no sound is output to the speakers.