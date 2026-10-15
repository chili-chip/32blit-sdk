# 2. Input

![Input](../assets/screenshots/input.png){ .screenshot align=right }

The VGC Zero has a d-pad and A, B, X, Y and MENU buttons. This lesson reads them three ways: held, just pressed, and just released.

```cpp title="input.cpp"
--8<-- "input/input.cpp"
```

## Held buttons

`buttons` holds the state of every button as bits, one per [`Button`](../api/input.md#button). Test a bit with `&`:

```cpp
if(buttons & Button::DPAD_LEFT)
  position.x -= speed;
```

This is true for every update while the button is held, so it's right for movement. `pressed(Button::A)` is the same test written as a function.

Test several buttons at once by combining them with `|`:

```cpp
if(buttons & (Button::A | Button::B))  // A or B held
  ...
```

## Pressed and released

For actions that should happen once per press, like jumping, firing or choosing a menu item, use `buttons.pressed`. It holds only the buttons that went down since the previous update:

```cpp
if(buttons.pressed & Button::A)
  jump();
```

`buttons.released` is the same for buttons that were let go. A charge-up attack might start on `pressed` and fire on `released`.

!!! warning "Read `pressed` and `released` in `update()`"
    They are worked out once per update, so in `update()` you see every press exactly once. In `render()` you might see the same press twice, or miss it, because render runs at a different rate.

## Smooth movement

`update()` runs 100 times a second, so moving 0.5 pixels per update is 50 pixels a second. Pixels are whole numbers, but positions don't have to be: keep positions in a [`Vec2`](../api/types.md#vec2) (floats) and convert to a `Point` when you draw. That way slow speeds work too.

```cpp
Vec2 position(60, 60);
...
screen.rectangle(Rect(Point(position), Size(8, 8)));
```

To move at the same speed diagonally, normalise the direction first:

```cpp
Vec2 dir(0, 0);
if(buttons & Button::DPAD_LEFT)  dir.x -= 1;
if(buttons & Button::DPAD_RIGHT) dir.x += 1;
if(buttons & Button::DPAD_UP)    dir.y -= 1;
if(buttons & Button::DPAD_DOWN)  dir.y += 1;

if(dir.x != 0 || dir.y != 0) {
  dir.normalize();
  position += dir * speed;
}
```

## Buttons the OS uses

On the console, **HOME** belongs to ChiliChip OS: a short press opens the system menu and holding it quits. Don't use it in your game. **MENU** is yours, and is the natural button for a pause menu.

## Key repeat

For menus, it's nice when holding a direction keeps moving the selection. Track when the button went down and repeat after a delay:

```cpp
uint32_t repeat_at = 0;

void update(uint32_t time) {
  if(buttons.pressed & Button::DPAD_DOWN) {
    select_next();
    repeat_at = time + 400;           // first repeat after 400 ms
  } else if((buttons & Button::DPAD_DOWN) && time >= repeat_at) {
    select_next();
    repeat_at = time + 100;           // then every 100 ms
  }
}
```

The built-in [`Menu`](../api/menu.md) class does this for you.

## The LED

The console's status LED is on whenever `LED` is set to a colour that isn't black. You can use it as a notification light:

```cpp
LED = Pen(255, 0, 0);   // on
LED = Pen(0, 0, 0);     // off
```

## Try it

* Make B shrink the square and X grow it.
* Make the square wrap around the screen edges instead of stopping.
* Add an 8-way normalised movement as shown above, and compare how diagonals feel.

Next: [sprites and assets](sprites.md).
