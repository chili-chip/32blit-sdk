// Audio: play notes on the synth channels.
//
// A, B, X and Y each play a note with a different waveform. Up and down
// change the octave.
#include "32blit.hpp"

using namespace blit;

const char *wave_names[] = {"Square", "Triangle", "Saw", "Sine"};
int octave = 0;
int last_channel = -1;

void init() {
  set_screen_mode(ScreenMode::hires);

  // channel 0: a short square-wave beep
  channels[0].waveforms = Waveform::SQUARE;
  channels[0].attack_ms = 5;
  channels[0].decay_ms = 100;
  channels[0].sustain = 0;   // no sustain, so the note ends by itself
  channels[0].release_ms = 0;

  // channel 1: a soft triangle that holds while the button is held
  channels[1].waveforms = Waveform::TRIANGLE;
  channels[1].attack_ms = 30;
  channels[1].decay_ms = 200;
  channels[1].sustain = 0xafff;
  channels[1].release_ms = 300;

  // channel 2: a buzzy saw
  channels[2].waveforms = Waveform::SAW;
  channels[2].attack_ms = 10;
  channels[2].decay_ms = 300;
  channels[2].sustain = 0x7fff;
  channels[2].release_ms = 100;

  // channel 3: a sine with a long release, like a bell
  channels[3].waveforms = Waveform::SINE;
  channels[3].attack_ms = 2;
  channels[3].decay_ms = 600;
  channels[3].sustain = 0;
  channels[3].release_ms = 600;
}

void play(int channel, uint16_t frequency) {
  channels[channel].frequency = frequency << octave;
  channels[channel].trigger_attack();
  last_channel = channel;
}

void update(uint32_t time) {
  const Button keys[] = {Button::A, Button::B, Button::X, Button::Y};
  const uint16_t notes[] = {262, 330, 392, 523}; // C4, E4, G4, C5

  for(int i = 0; i < 4; i++) {
    if(buttons.pressed & keys[i])
      play(i, notes[i]);

    // letting go moves a held note on to its release
    if(buttons.released & keys[i])
      channels[i].trigger_release();
  }

  if(buttons.pressed & Button::DPAD_UP)   octave = std::min(octave + 1, 2);
  if(buttons.pressed & Button::DPAD_DOWN) octave = std::max(octave - 1, 0);
}

void render(uint32_t time) {
  screen.pen = Pen(20, 20, 30);
  screen.clear();

  screen.pen = Pen(255, 255, 255);
  screen.text("A B X Y: play notes", minimal_font, Point(4, 4));
  screen.text("Up/Down: octave " + std::to_string(octave), minimal_font, Point(4, 14));

  // one bar per channel showing its envelope level
  for(int i = 0; i < 4; i++) {
    int y = 34 + i * 22;
    // adsr is the envelope level, 24-bit
    int level = channels[i].adsr >> 18; // 0..63
    if(channels[i].adsr_phase == ADSRPhase::OFF)
      level = 0;

    screen.pen = Pen(60, 60, 80);
    screen.rectangle(Rect(50, y, 70, 12));
    screen.pen = hsv_to_rgba(i / 4.0f, 0.7f, 1.0f);
    screen.rectangle(Rect(50, y, level + 6 * (level > 0), 12));

    screen.pen = i == last_channel ? Pen(255, 255, 255) : Pen(150, 150, 150);
    screen.text(wave_names[i], minimal_font, Point(4, y + 2));
  }
}
