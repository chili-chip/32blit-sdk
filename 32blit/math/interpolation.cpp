/*! \file interpolation.cpp
*/
#include "interpolation.hpp"

namespace blit {
  /**
   * Map `value` from the range [`start`, `end`] onto the range [`min`, `max`].
   *
   * `value` is first clamped to [`start`, `end`], so the result never leaves [`min`, `max`]
   * (assuming `start` < `end`). When `start == end` the result is `min`.
   *
   * @param value Input value, in the same units as `start` and `end`.
   * @param start Lower bound of the input range; maps to `min`.
   * @param end Upper bound of the input range; maps to `max`.
   * @param min Output value when `value` is `start`.
   * @param max Output value when `value` is `end`.
   * @return `((clamp(value, start, end) - start) / (end - start)) * (max - min) + min`.
   */
  float lerp(float value, float start, float end, float min, float max) {
    if (end == start)
      return min;
    value = value < start ? start : (value > end ? end : value);
    return (((value - start) / (end - start)) * (max - min)) + min;
  }

  /**
   * Find how far `value` lies between `start` and `end` (an "inverse lerp").
   *
   * Returns `0.0f` when `value == start` and `1.0f` when `value == end`. The result is
   * not clamped, so values outside [`start`, `end`] give results below `0` or above `1`.
   * Passing `start == end` divides by zero.
   *
   * @param value Input value.
   * @param start Value that maps to `0.0f`.
   * @param end Value that maps to `1.0f`.
   * @return `(value - start) / (end - start)`.
   */
  float lerp(float value, float start, float end) {
    return (value - start) / (end - start);
  }

  /**
   * Map `value` from the range [`start`, `end`] onto the line between the vectors `min` and `max`.
   *
   * Vector version of lerp(float, float, float, float, float), with the same behaviour:
   * `value` is clamped to [`start`, `end`], and `start == end` gives `min`.
   *
   * @param value Input value, in the same units as `start` and `end`.
   * @param start Lower bound of the input range; maps to `min`.
   * @param end Upper bound of the input range; maps to `max`.
   * @param min Result when `value` is `start`.
   * @param max Result when `value` is `end`.
   * @return `(max - min) * ((clamp(value, start, end) - start) / (end - start)) + min`.
   */
  Vec2 lerp(float value, float start, float end, Vec2 min, Vec2 max) {
    if (end == start)
      return min;
    value = value < start ? start : (value > end ? end : value);
    return ((max - min) * ((value - start) / (end - start))) + min;
  }

  /**
   * Linearly interpolate between two vectors.
   *
   * Unlike the float overload lerp(float, float, float), which returns a factor, this
   * takes the factor and returns the point. `value` is not clamped, so values outside
   * [`0`, `1`] extrapolate beyond `start` or `end`.
   *
   * @param value Interpolation factor: `0.0f` gives `start`, `1.0f` gives `end`.
   * @param start Vector returned when `value` is `0.0f`.
   * @param end Vector returned when `value` is `1.0f`.
   * @return `start + (end - start) * value`.
   */
  Vec2 lerp(float value, Vec2 start, Vec2 end) {
    return ((end - start) * value) + start;
  }
}