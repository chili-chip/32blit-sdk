/*! \file mode7.cpp
    \brief Emulate mode7 graphics effects.

    Functions to emulate the mode7 graphics effect from classic consoles.
*/
#include <cmath>
#include <cfloat>

#include "../math/interpolation.hpp"
#include "mode7.hpp"

#include "../types/mat3.hpp"
#include "../graphics/font.hpp"

namespace blit {
 

  // TODO: Provide method to return scale for world coordinate

  /**
   * Convert a mode7 world-space coordinate to a screen coordinate.
   *
   * This is the inverse of @ref screen_to_world, using the same camera parameters. The point's
   * depth along the camera's forward direction gives the screen row, and its sideways offset
   * relative to the width of the field of view at that depth gives the screen column.
   *
   * The returned x is `0` at the left edge of the field of view and `viewport.w` at the right
   * edge; `viewport.x` is not added. The returned y is `viewport.y + (far - near) / (d - near)`,
   * where `d` is the distance along the edge of the field of view at the point's depth.
   *
   * The result is only meaningful for points in front of the camera. A point at `pos`
   * gives NaN, and points behind the camera give meaningless values.
   *
   * \param[in] w World-space coordinate to convert.
   * \param[in] fov Camera field-of-view, in radians.
   * \param[in] angle Camera rotation in world space, in radians. At `0` the camera faces -y.
   * \param[in] pos Camera position in world space.
   * \param[in] near Distance the rows approach as they move away from the top of the viewport.
   * \param[in] far Distance at the row one pixel below the top of the viewport.
   * \param[in] viewport Screen rectangle the mode7 view is drawn in.
   * \return Screen coordinate of `w`.
   */
  Vec2 world_to_screen(Vec2 w, float fov, float angle, Vec2 pos, float near, float far, Rect viewport) {
    float hfov = fov / 2.0f;    

    Vec2 v(w - pos);
    v.normalize();
    Vec2 f(0, -1);
    f *= Mat3::rotation(angle);

    float wd = (w - pos).length();
    float dot = f.dot(v);
    float det = f.x * v.y - f.y * v.x;
    float theta = atan2f(det, dot);
    float costheta = cosf(theta);
    float ctd = wd * costheta;

    float pd = ctd / cosf(hfov);

    float theta_sign = std::copysign(1.0f, theta);
    
    float hsl = sqrtf((pd * pd) - (ctd * ctd));
    float so = hsl + (sqrtf((wd * wd) - (ctd * ctd)) * theta_sign);
    float r = so / (hsl * 2.0f);

    Vec2 s(
      r * viewport.w,
      ((far - near) / (pd - near)) + viewport.y
    );
      
    return s;
  }

  /**
   * Convert a screen coordinate to a mode7 world-space coordinate.
   * 
   * \param[in] s vec2 describing the screen coordinate
   * \param[in] fov Current camera field-of-view
   * \param[in] angle Current camera z-angle in mode7 world-space
   * \param[in] pos Current camera position in mode7 world-space
   * \param[in] near Distance to nearest visible point
   * \param[in] far Distance to furthest visible point
   * \param[in] viewport
   */
  Vec2 screen_to_world(Vec2 s, float fov, float angle, Vec2 pos, float near, float far, Rect viewport) {
    Vec2 forward(0, -1);
    forward *= Mat3::rotation(angle);

    Vec2 left = forward;
    left *= Mat3::rotation(-(fov / 2.0f));

    Vec2 right = forward;
    right *= Mat3::rotation((fov / 2.0f));

    float distance = ((far - near) / float(s.y - viewport.y)) + near;
    
    Vec2 swc = pos + (left * distance);
    Vec2 ewc = pos + (right * distance);

    return lerp(s.x, viewport.x, viewport.x + viewport.w, swc, ewc);
  }


  // TODO: Add support for a default tile to draw outside of the bounds of the map and for the map to be repeated.

  /**
   * Draw a map layer as a mode7 perspective ground plane.
   *
   * For every row of `viewport`, @ref screen_to_world gives the world coordinates under the
   * left and right edges of the row, and `layer->mipmap_texture_span` draws the span between
   * them. Rows are drawn into `dest` directly; `viewport` is not clipped against `dest->clip`.
   *
   * The textures come from `sprites->mipmaps`, which must not be empty (see
   * @ref Surface::generate_mipmaps). The mipmap level for each row is picked from how many
   * world pixels it covers per screen pixel, and the next level is blended over it using
   * `dest->alpha`. `dest->alpha` is not restored: it is left at the last row's blend amount
   * (or 255 when there is only one mipmap level).
   *
   * The horizontal mapping is only correct when `viewport.x` is `0`, because @ref screen_to_world
   * uses the 5-argument `lerp`, which does not subtract its `start` value.
   *
   * After drawing, this also sets `dest->pen` to magenta and draws a single debug pixel at the
   * screen position of world coordinate (400, 400).
   *
   * \param[in] dest Surface to draw to.
   * \param[in] sprites Sprite sheet with generated mipmaps, used for the tile textures.
   * \param[in] layer Map layer to draw. Its `transforms` must have an entry for every tile.
   * \param[in] fov Camera field-of-view, in radians.
   * \param[in] angle Camera rotation in world space, in radians. At `0` the camera faces -y.
   * \param[in] pos Camera position in world space.
   * \param[in] near Distance the rows approach as they move away from the top of the viewport.
   * \param[in] far Distance at the row one pixel below the top of the viewport.
   * \param[in] viewport Screen rectangle to draw in. The top row is at infinite distance.
   */
  void mode7(Surface *dest, Surface *sprites, MapLayer *layer, float fov, float angle, Vec2 pos, float near, float far, Rect viewport) {
    for (int y = viewport.y; y < viewport.y + viewport.h; y++) {
      Vec2 swc = screen_to_world(Vec2(viewport.x, y), fov, angle, pos, near, far, viewport);
      Vec2 ewc = screen_to_world(Vec2(viewport.x + viewport.w, y), fov, angle, pos, near, far, viewport);

      layer->mipmap_texture_span(
        dest,
        Point(viewport.x, y),
        viewport.w,
        sprites,
        swc,
        ewc);
    }

    Vec2 s = world_to_screen(Vec2(400, 400), fov, angle, pos, near, far, viewport);
    dest->pen = Pen(255, 0, 255);
    dest->pixel(s);
  }
  
}
