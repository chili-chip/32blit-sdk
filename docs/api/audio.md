# Audio

`audio/audio.hpp`, `audio/wav-stream.hpp`, `audio/mp3-stream.hpp`

The SDK mixes 8 synthesiser channels into one mono output at 22 050 samples a second. On the VGC Zero the mixing runs on the second core, from RAM, and comes out of the speaker through PWM. See the [sound tutorial](../tutorials/sound.md).

## Globals

```cpp
#define CHANNEL_COUNT 8
const uint32_t sample_rate = 22050;

extern AudioChannel * const &channels;   // channels[0] .. channels[7]
extern uint16_t volume;                  // master volume, 0 - 0xffff

uint16_t get_audio_frame();
bool is_audio_playing();
```

`channels`
:   The eight channels. Set them up and trigger them directly.

`volume`
:   The master volume. On the VGC Zero it follows the player's volume setting, so leave it alone and balance your sounds with each channel's `volume` instead.

`is_audio_playing()`
:   `true` if any channel is sounding (and the master volume isn't 0).

`get_audio_frame()`
:   Mixes and returns the next output sample. The firmware calls it; games don't need to.

## `AudioChannel`

```cpp
struct AudioChannel {
  uint8_t   waveforms     = 0;       // which Waveform bits are on
  uint16_t  frequency     = 660;     // Hz
  uint16_t  volume        = 0xffff;  // channel volume

  uint16_t  attack_ms     = 2;
  uint16_t  decay_ms      = 6;
  uint16_t  sustain       = 0xffff;  // sustain level
  uint16_t  release_ms    = 1;
  uint16_t  pulse_width   = 0x7fff;  // square wave duty cycle, 50% by default

  bool      filter_enable = false;
  uint16_t  filter_cutoff_frequency = 0;

  uint32_t  adsr;                    // current envelope level (24-bit)
  ADSRPhase adsr_phase;              // ATTACK, DECAY, SUSTAIN, RELEASE or OFF

  int16_t   wave_buffer[64];         // samples for Waveform::WAVE
  uint8_t   wave_buf_pos;
  void (*wave_buffer_callback)(AudioChannel &channel);
  void *user_data;

  void trigger_attack();
  void trigger_decay();
  void trigger_sustain();
  void trigger_release();
  void off();
};
```

### Sound

`waveforms`
:   One or more [`Waveform`](#waveform) bits. With several bits set, the waveforms are averaged.

`frequency`
:   The pitch in Hz. Change it while a note plays for slides and vibrato.

`volume`
:   The channel's loudness, `0` to `0xffff`.

`pulse_width`
:   For `SQUARE`, how much of each cycle is high, `0` to `0xffff`. `0x7fff` is a square wave; smaller values sound thinner and more nasal.

`filter_enable`, `filter_cutoff_frequency`
:   A simple filter on the channel. **Ignored on the VGC Zero**, where it would slow the audio core; it only works on the desktop.

### Envelope

`attack_ms`, `decay_ms`, `sustain`, `release_ms`
:   The ADSR envelope: rise to full volume over `attack_ms`, fall to the `sustain` level over `decay_ms`, hold there until released, then fade to silence over `release_ms`.

`trigger_attack()`
:   Starts a note from its current level. The envelope then moves on to decay and sustain by itself.

`trigger_release()`
:   Ends a held note: fades out over `release_ms`.

`off()`
:   Silences the channel immediately.

`trigger_decay()`, `trigger_sustain()`
:   Jump to those phases; the mixer calls them as the envelope progresses.

`adsr`, `adsr_phase`
:   The envelope's current level (0 to `0xffffff`) and phase, for drawing meters or checking whether a note has finished (`adsr_phase == ADSRPhase::OFF`).

A `sustain` of `0` makes a note end by itself after its decay, which suits sound effects.

### Custom waveforms

`wave_buffer`, `wave_buf_pos`, `wave_buffer_callback`, `user_data`
:   With `Waveform::WAVE`, the channel plays the 64 samples in `wave_buffer` and calls `wave_buffer_callback` each time it reaches the end, to refill the buffer. `user_data` is yours to point at your own state. This is how `WavStream` and `MP3Stream` work.

The callback runs on the audio core in the middle of mixing, so it must be quick and must not draw or allocate. On the VGC Zero it should also avoid running code from flash where possible; mark it with [`blit_fast_code`](engine.md#fast-code).

## `Waveform`

```cpp
enum Waveform {
  NOISE    = 128,
  SQUARE   = 64,
  SAW      = 32,
  TRIANGLE = 16,
  SINE     = 8,
  WAVE     = 1,  // samples from wave_buffer
};
```

## `ADSRPhase`

```cpp
enum class ADSRPhase : uint8_t { ATTACK, DECAY, SUSTAIN, RELEASE, OFF };
```

## `WavStream` and `MP3Stream`

```cpp
class WavStream {
public:
  enum PlayFlags { from_start = 1, loop = 2 };

  bool load(std::string filename);
  void play(int channel, int flags = 0);
  void pause();
  void restart();
  void update();

  bool get_playing() const;
  int get_play_flags() const;
  unsigned int get_current_sample() const;
  int get_duration_ms() const;
};

class MP3Stream {
public:
  enum PlayFlags { from_start = 1, loop = 2 };

  bool load(std::string filename, bool do_duration_calc = false);
  // the rest as WavStream
};
```

Both stream a sound file through one channel, reading and decoding a little at a time.

`load(filename)`
:   Opens a file from [storage](files.md) or an in-memory file registered with `File::add_buffer_file()`. Returns `false` if it can't be opened or isn't a supported format. `MP3Stream::load` can also work out the duration up front (`do_duration_calc`), which reads the whole file.

`play(channel, flags)`
:   Plays on a channel, taking it over (it sets the channel's waveform to `WAVE`). `from_start` restarts from the beginning; `loop` repeats at the end.

`pause()`, `restart()`
:   Pause, and go back to the start.

`update()`
:   Reads and decodes the next part of the file. **Call it every update** while the stream plays.

`get_playing()`, `get_current_sample()`, `get_duration_ms()`
:   Whether it's playing, the position in samples, and the length.

### Formats

| Stream | Supported files |
| ------ | --------------- |
| `WavStream` | uncompressed PCM, mono, 22 050 or 11 025 Hz |
| `MP3Stream` | any MP3; mono 22 050 Hz plays as is, other rates and stereo are converted (roughly) |

MP3 decoding runs in `update()` on your game's core and takes noticeable time on the console. WAV costs almost nothing to play but takes more space.

### Shipping sounds in the game

```yaml title="assets.yml"
assets.cpp:
  assets/jingle.wav:
    name: asset_jingle
    type: raw/binary
```

```cpp
WavStream jingle;

void init() {
  File::add_buffer_file("jingle.wav", asset_jingle, asset_jingle_length);
  jingle.load("jingle.wav");
}

void play_jingle() {
  jingle.play(6, WavStream::from_start);
}

void update(uint32_t time) {
  jingle.update();
}
```
