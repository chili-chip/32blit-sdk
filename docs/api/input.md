# Input

`engine/input.hpp`, `engine/api.hpp`

See the [input tutorial](../tutorials/input.md) for examples.

## `buttons`

```cpp
extern ButtonState &buttons;

struct ButtonState {
  uint32_t state;      // buttons held now
  uint32_t pressed;    // buttons that went down since the previous update
  uint32_t released;   // buttons that went up since the previous update

  operator uint32_t() const;   // same as state
};
```

Each field is a set of [`Button`](#button) bits.

`state` (or `buttons` itself)
:   The buttons held right now. Test with `buttons & Button::A`.

`pressed`, `released`
:   The buttons that changed since the previous call to `update()`. They are worked out once per update, so read them in `update()`: there you see every press and release exactly once.

```cpp
bool pressed(uint32_t button);
```

`true` if any of the given buttons is held: the same as `buttons.state & button`. Despite the name, it reports held buttons, not new presses; use `buttons.pressed` for those.

## `Button`

```cpp
enum Button : unsigned int {
  DPAD_LEFT  = 1,
  DPAD_RIGHT = 2,
  DPAD_UP    = 4,
  DPAD_DOWN  = 8,
  A          = 16,
  B          = 32,
  X          = 64,
  Y          = 128,
  MENU       = 256,
  HOME       = 512,
  JOYSTICK   = 1024,  // pressing the analog stick, on hardware that has one
};
```

On the VGC Zero:

| Button | Notes |
| ------ | ----- |
| D-pad, A, B, X, Y | yours to use |
| MENU | yours to use; a good pause button |
| HOME | reserved by ChiliChip OS for the system menu and quitting. It shows in `buttons` while held, but don't use it. |
| JOYSTICK | never set: the console has no analog stick |

Buttons are debounced by the firmware, so one physical press gives one `pressed` event.

Desktop key mappings are listed in [Run on your computer](../getting-started/build-desktop.md#controls).

## Analog input

```cpp
extern Vec2 &joystick;
extern Vec3 &tilt;
extern float &hack_left;
extern float &hack_right;
```

`joystick`
:   The analog stick, `x` and `y` from -1 to 1. Always zero on the VGC Zero; on the desktop it follows a game controller's left stick.

`tilt`
:   An accelerometer reading. Always zero on the VGC Zero.

`hack_left`, `hack_right`
:   Readings from the 32blit handheld's hack header. Not used on the VGC Zero.

## Outputs

```cpp
extern Pen &LED;
extern float &vibration;
```

`LED`
:   The status LED's colour. The VGC Zero has a single-colour LED on GPIO 16: it lights whenever any of `r`, `g` or `b` is above 0, and alpha is ignored.

    ```cpp
    LED = Pen(255, 0, 0);  // on
    LED = Pen(0, 0, 0);    // off
    ```

`vibration`
:   Rumble strength from 0 to 1, on hardware with a vibration motor. The VGC Zero has none.
