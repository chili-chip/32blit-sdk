#include <algorithm>
#include <cstdio>
#include <iostream>
#include "SDL.h"

#include "Audio.hpp"
#include "audio/audio.hpp"
#include "engine/api_private.hpp"

static void _audio_callback(void *userdata, uint8_t *stream, int len);

Audio::Audio() {
    SDL_AudioSpec desired = {}, audio_spec = {};

    desired.freq = _sample_rate;
    desired.format = AUDIO_S16LSB;
    desired.channels = 1;

    desired.samples = 256;
    desired.callback = _audio_callback;
    desired.userdata = this;

    audio_device = SDL_OpenAudioDevice(nullptr, 0, &desired, &audio_spec, 0);

    if(audio_device == 0){
        std::cerr << "Audio Init Failed: " << SDL_GetError() << std::endl;
        return;
    }

    SDL_PauseAudioDevice(audio_device, 0);
}

Audio::~Audio() {
    if(audio_device == 0) return;
    SDL_PauseAudioDevice(audio_device, 1);
    SDL_CloseAudioDevice(audio_device);
}

static void _audio_bufferfill(short *buffer, int buffer_size){
    if(buffer) {
        memset(buffer, 0, buffer_size * sizeof(short));
    }

    for(auto sample = 0; sample < buffer_size; sample++){
        // Always get audio frame to update audio state
        int16_t frame = (int)blit::get_audio_frame() - 0x8000;
        if(buffer) {
            buffer[sample] = frame;
        }
    }
}

static void _audio_callback(void *userdata, uint8_t *stream, int len){
    Audio *audio = static_cast<Audio *>(userdata);
    _audio_bufferfill(audio->is_muted() ? nullptr : (short *)stream, len / 2);
}
