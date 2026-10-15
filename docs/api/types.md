# Maths and types

`types/point.hpp`, `types/size.hpp`, `types/rect.hpp`, `types/vec2.hpp`, `types/vec3.hpp`, `types/mat3.hpp`, `types/mat4.hpp`, `math/constants.hpp`, `math/interpolation.hpp`

Whole-pixel types (`Point`, `Size`, `Rect`) are for drawing; float types (`Vec2`, `Vec3`) are for movement and physics. Convert with `Point(vec)` when you draw.

## `Point`

```cpp
struct Point {
  int32_t x = 0, y = 0;

  Point() = default;
  constexpr Point(int32_t x, int32_t y);
  constexpr Point(Vec2 v);                   // truncates towards zero
  explicit constexpr operator Vec2() const;
};
```

A pixel position. Points convert from `Vec2` automatically (the fractions are dropped), so you can pass a `Vec2` wherever drawing functions want a `Point`. Converting the other way needs `Vec2(p)`.

| Operator | Result |
| -------- | ------ |
| `p + q`, `p - q`, `-p` | component-wise |
| `p * f` | both components multiplied by the float `f`, truncated |
| `p / n` | both divided by the integer `n` (integer division) |
| `p * m` | transformed by the `Mat3` `m` |
| `p == q`, `p != q` | compare |

`+=`, `-=`, `*=` and `/=` work the same way in place.

## `Size`

```cpp
struct Size {
  int32_t w = 0, h = 0;

  Size() = default;
  constexpr Size(int32_t w, int32_t h);

  constexpr bool empty() const;              // w <= 0 or h <= 0
  constexpr int32_t area() const;            // w * h
  constexpr bool contains(const Point &p) const;  // 0 <= x < w and 0 <= y < h
};
```

A width and height. Supports `*` and `/` by an `int` or a `float`, and `==`/`!=`. `screen.bounds` is a `Size`.

## `Rect`

```cpp
struct Rect {
  int32_t x = 0, y = 0, w = 0, h = 0;

  Rect() = default;
  constexpr Rect(int32_t x, int32_t y, int32_t w, int32_t h);
  constexpr Rect(Point tl, Size s);
  constexpr Rect(Point tl, Point br);   // br is one past the bottom-right pixel
};
```

A rectangle by its top-left corner and size. It covers the pixels from `x` to `x + w - 1` and `y` to `y + h - 1`.

| Member | Returns |
| ------ | ------- |
| `size()` | `Size(w, h)` |
| `empty()` | `true` if `w` or `h` is 0 or less |
| `contains(Point p)` | `true` if `p` is one of the rectangle's pixels |
| `contains(Rect r)` | `true` if `r` is inside, not touching the right or bottom edge |
| `intersects(Rect r)` | `true` if the rectangles overlap **or touch** (see below) |
| `intersection(Rect r)` | the overlapping rectangle; check `empty()` on the result, because rectangles that don't overlap give a negative size |
| `inflate(v)`, `deflate(v)` | grow or shrink by `v` on every side, in place |
| `clamp(Point p)` | the nearest point to `p` from `x`..`x + w`, `y`..`y + h` |
| `tl()` | `Point(x, y)` |
| `tr()`, `bl()`, `br()` | `Point(x + w, y)`, `Point(x, y + h)`, `Point(x + w, y + h)`: one past the last pixel |
| `center()` | `Point(x + w/2, y + h/2)` |

`Rect * f` scales all four fields; `==` and `!=` compare.

`intersects()` counts rectangles that only share an edge as intersecting: `Rect(0, 0, 8, 8)` and `Rect(8, 0, 8, 8)` intersect, though no pixel is in both. That's usually harmless for game collisions. For a strict pixel overlap, use `!a.intersection(b).empty()`.

## `Vec2`

```cpp
struct Vec2 {
  float x, y;

  explicit constexpr Vec2(float x = 0, float y = 0);

  float length();
  void normalize();                    // makes length 1, in place
  float dot(const Vec2 &a);
  float cross(const Vec2 &a);          // x * a.y - y * a.x
  void rotate(float radians);          // in place
  float angle(Vec2 o);                 // signed angle from this to o, radians
  float angle_to(Vec2 o);              // direction of (this - o), radians
  void transform(const Mat3 &m);       // in place

  static Vec2 normalize(Vec2 &v);      // normalizes v in place and returns it
};
```

A 2D vector of floats for positions, velocities and directions.

* Operators: `+`, `-`, unary `-`; `* float`, `/ float`; `* Vec2` and `/ Vec2` (component-wise); `* Mat3` (transform), and the in-place forms.
* The constructor is `explicit`, so write `Vec2(1, 2)`, not `{1, 2}`, when passing or assigning one.
* `normalize()` divides by the length, so a zero vector becomes NaN. Check the length first:

```cpp
Vec2 dir(0, 0);
if(buttons & Button::DPAD_LEFT)  dir.x -= 1;
if(buttons & Button::DPAD_RIGHT) dir.x += 1;
if(buttons & Button::DPAD_UP)    dir.y -= 1;
if(buttons & Button::DPAD_DOWN)  dir.y += 1;
if(dir.length() > 0) {
  dir.normalize();          // diagonals no faster than straight lines
  player += dir * speed;
}
```

* `rotate()` and `Mat3::rotation()` use radians, clockwise on screen, because y points down.
* `angle_to(o)` is the direction pointing from `o` to this vector. To aim from `a` at `b`, use `b.angle_to(a)`, or `atan2f(b.y - a.y, b.x - a.x)`.

## `Mat3`

```cpp
struct Mat3 {
  float v00, v01, v02;
  float v10, v11, v12;
  float v20, v21, v22;

  Mat3();                         // all zeros, not identity
  static Mat3 identity();
  static Mat3 rotation(float radians);
  static Mat3 translation(Vec2 v);
  static Mat3 scale(Vec2 v);
  void inverse();                 // in place
};
```

A 3×3 matrix for 2D transforms: used for tile map drawing and for transforming points and vectors.

!!! warning
    `Mat3()` is all zeros. Start from `Mat3::identity()` or one of the other constructors.

### Combining transforms

`m1 * m2` multiplies matrices, and `v * m` transforms a vector or point by `m`. In a product, **the rightmost matrix is applied first**:

```cpp
// rotate around (64, 64): move it to the origin, rotate, move it back
Mat3 m = Mat3::translation(Vec2(64, 64))
       * Mat3::rotation(angle)
       * Mat3::translation(Vec2(-64, -64));

Vec2 p = Vec2(100, 64) * m;
```

`v * m1 * m2` is different: it applies `m1` first, then `m2`, because it's `(v * m1) * m2`.

`inverse()` turns a transform into the one that undoes it, for example to turn a tile map's screen-to-world transform into world-to-screen for drawing sprites on the map.

## `Vec3` and `Mat4`

```cpp
struct Vec3 {
  float x, y, z;
  constexpr Vec3(float x = 0, float y = 0, float z = 0);

  float length();
  void normalize();
  Vec3 cross(const Vec3 &a);
  float dot(const Vec3 &a);
  void transform(const Mat4 &m);     // with perspective divide
};

struct Mat4 {
  float v00 ... v33;
  Mat4();                            // all zeros
  static Mat4 identity();
  static Mat4 rotation(float degrees, Vec3 axis);
  static Mat4 translation(Vec3 v);
  static Mat4 scale(Vec3 v);
  void inverse();
};
```

The 3D versions, for simple 3D effects such as rotating wireframes or star fields. The operators match `Vec2` and `Mat3`.

!!! note
    `Mat4::rotation` takes its angle in **degrees**, unlike `Mat3::rotation` and `Vec2::rotate`, which use radians.

`transform()` divides by the resulting `w`, so a projection matrix gives screen-space coordinates directly.

## Constants and interpolation

```cpp
constexpr float pi = 3.14159265358979323846f;
```

Use `pi` for angles: `pi * 2` is a full turn, `pi / 180` converts degrees to radians.

The `lerp()` functions are described on the [timers and tweens](timers-tweens.md#interpolation-helpers) page. Note that the float and `Vec2` three-argument forms do opposite things.
