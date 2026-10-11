/*! \file tilemap.cpp
*/
#include <cstring>
#include "tilemap.hpp"

namespace blit {

  /**
   * Create a new tilemap.
   *
   * \param[in] tiles
   * \param[in] transforms
   * \param[in] bounds Map bounds, must be a power of two
   * \param[in] sprites
   */
  TileMap::TileMap(uint8_t *tiles, uint8_t *transforms, Size bounds, Surface *sprites) : bounds(bounds), tiles(tiles), transforms(transforms), sprites(sprites) {
  }

  TileMap *TileMap::load_tmx(const uint8_t *asset, Surface *sprites, int layer, int flags) {
    auto map_struct = reinterpret_cast<const TMX *>(asset);

    if(memcmp(map_struct, "MTMX", 4) != 0 || map_struct->header_length != sizeof(TMX))
      return nullptr;

    // only 8 bit tiles supported
    if(map_struct->flags & TMX_16Bit)
      return nullptr;

    // power of two bounds required
    if((map_struct->width & (map_struct->width - 1)) || (map_struct->height & (map_struct->height - 1)))
      return nullptr;

    auto layer_size = map_struct->width * map_struct->height;

    uint8_t *tile_data;
    if(flags & copy_tiles) {
      tile_data = new uint8_t[layer_size];
      memcpy(tile_data, map_struct->data + layer_size * layer, layer_size);
    } else {
      tile_data = const_cast<uint8_t *>(map_struct->data + layer_size * layer);
    }

    auto transform_base = map_struct->data + layer_size * map_struct->layers;

    uint8_t *transform_data = nullptr;

    if(flags & copy_transforms) {
      transform_data = new uint8_t[layer_size]();

      if(map_struct->flags & TMX_Transforms)
        memcpy(transform_data, transform_base + layer_size * layer, layer_size);
    } else if(map_struct->flags & TMX_Transforms) {
      transform_data = const_cast<uint8_t *>(transform_base + layer_size * layer);
    }

    auto ret = new TileMap(tile_data, transform_data, Size(map_struct->width, map_struct->height), sprites);
    ret->empty_tile_id = map_struct->empty_tile;

    return ret;
  }

  /**
   * Get the index into `tiles` and `transforms` for a tile position, applying `repeat_mode`.
   *
   * Positions inside the map give `x + y * bounds.w`. For positions outside the map the result
   * depends on `repeat_mode`:
   * - `NONE`: `-1`.
   * - `REPEAT`: the position is wrapped around the map (this relies on `bounds` being a power of two).
   * - `CLAMP_TO_EDGE`: the position is clamped to the nearest edge tile.
   * - `DEFAULT_FILL`: `default_tile_id` is returned as-is. Note that this is a tile id, not an
   *   index, so callers that use the result to index `tiles` read whatever tile is stored at
   *   index `default_tile_id`.
   *
   * \param[in] x Tile x position (in tiles, not pixels).
   * \param[in] y Tile y position (in tiles, not pixels).
   * \return Index into `tiles`/`transforms`, or `-1` if the position is outside the map and `repeat_mode` is `NONE`.
   */
  int32_t TileMap::offset(int16_t x, int16_t y) {
    int32_t cx = ((uint32_t)x) & (bounds.w - 1);
    int32_t cy = ((uint32_t)y) & (bounds.h - 1);

    if ((x ^ cx) | (y ^ cy)) {
      if (repeat_mode == DEFAULT_FILL)
        return default_tile_id;

      if (repeat_mode == REPEAT)
        return cx + cy * bounds.w;

      if(repeat_mode == CLAMP_TO_EDGE) {
        if(x != cx)
          cx = x < 0 ? 0 : bounds.w - 1;
        if(y != cy)
          cy = y < 0 ? 0 : bounds.h - 1;

        return cx + cy * bounds.w;
      }

      return -1;
    }

    return cx + cy * bounds.w;
  }

  /**
   * Get flags for a specific tile.
   *
   * \param[in] p Point denoting the tile x/y position in the map.
   * \return Bitmask of flags for specified tile.
   */
  uint8_t TileMap::tile_at(const Point &p) {
    int32_t o = offset(p.x, p.y);

    if(o != -1)
      return tiles[o];

    return 0;
  }

  /**
   * Get transform for a specific tile.
   *
   * \param[in] p Point denoting the tile x/y position in the map.
   * \return Bitmask of transforms for specified tile.
   */
  uint8_t TileMap::transform_at(const Point &p) {
    int32_t o = offset(p.x, p.y);

    if (o != -1 && transforms)
      return transforms[o];

    return 0;
  }

  /**
   * Draw tilemap to a specified destination surface, with clipping.
   *
   * \param[in] dest Destination surface.
   * \param[in] viewport Clipping rectangle.
   * \param[in] scanline_callback Functon called on every scanline, accepts the scanline y position, should return a transformation matrix.
   */
  void TileMap::draw(Surface *dest, Rect viewport, std::function<Mat3(uint8_t)> scanline_callback) {
    //bool not_scaled = (from.w - to.w) | (from.h - to.h);

    viewport = dest->clip.intersection(viewport);

    for (uint16_t y = viewport.y; y < viewport.y + viewport.h; y++) {
      Vec2 swc(viewport.x, y);
      Vec2 ewc(viewport.x + viewport.w, y);

      if (scanline_callback) {
        Mat3 custom_transform = scanline_callback(y);
        swc *= custom_transform;
        ewc *= custom_transform;
      } else {
        swc *= transform;
        ewc *= transform;
      }

      texture_span(dest, Point(viewport.x, y), viewport.w, swc, ewc);
    }
  }

  /*
  void tilemap::mipmap_texture_span(surface *dest, point s, uint16_t c, vec2 swc, vec2 ewc) {
    // calculate the mipmap index to use for drawing
    float span_length = (ewc - swc).length();
    float mipmap = ((span_length / float(c)) / 2.0f);
    int16_t mipmap_index = floor(mipmap);
    uint8_t blend = (mipmap - floor(mipmap)) * 255;

    mipmap_index = mipmap_index >= (int)sprites->s.mipmaps.size() ? sprites->s.mipmaps.size() - 1 : mipmap_index;
    mipmap_index = mipmap_index < 0 ? 0 : mipmap_index;

    dest->alpha = 255;
    texture_span(dest, s, c, swc, ewc, mipmap_index);

    if (++mipmap_index < sprites->s.mipmaps.size()) {
      dest->alpha = blend;
      texture_span(dest, s, c, swc, ewc, mipmap_index);
    }
  }*/

  /**
   * Draw one horizontal span of the tilemap, sampling the map along a line in world space.
   *
   * Pixel `i` of the span (`0 <= i < c`) is drawn at `s + (i, 0)` with the colour of the
   * world-space pixel `swc + (ewc - swc) * i / c`, so `ewc` is the world position one pixel past
   * the end of the span. This is what @ref draw calls once per scanline, and it allows the map
   * to be scaled, rotated or skewed per scanline.
   *
   * World coordinates are in pixels, with each tile 8x8 pixels. Tile ids index `sprites` as a
   * sheet of 8x8 sprites, 16 per row, so the sheet is expected to be 128 pixels wide. Bits in
   * the tile's `transforms` entry change the sampled pixel: `0b100` flips it horizontally,
   * `0b010` flips it vertically and `0b001` swaps x and y (after the flips). Pixels are written
   * with `dest`'s pen blend function, so `dest->alpha` and `dest->mask` apply, and fully
   * transparent sprite pixels are skipped by the RGB/RGBA blend functions.
   *
   * Pixels over tiles equal to `empty_tile_id`, or outside the map when `repeat_mode` is `NONE`,
   * are left unchanged.
   *
   * The span is not clipped and sprite coordinates are not bounds checked: `s` and `c` must lie
   * inside `dest` and every tile id must be inside `sprites`. Positions are stepped in 16.16
   * fixed point, so world coordinates must stay within +/-32767 pixels.
   *
   * \param[in] dest Surface to draw to.
   * \param[in] s Screen position of the first pixel of the span.
   * \param[in] c Number of pixels to draw. Must be at least 1.
   * \param[in] swc World coordinate (in pixels) sampled for the first pixel.
   * \param[in] ewc World coordinate (in pixels) one pixel past the end of the span.
   */
  void TileMap::texture_span(Surface *dest, Point s, unsigned int c, Vec2 swc, Vec2 ewc) {
    Surface *src = sprites;

    static const int fix_shift = 16;

    Point wc(swc * (1 << fix_shift));
    Point dwc(((ewc - swc) / float(c)) * (1 << fix_shift));
    int32_t doff = dest->offset(s.x, s.y);

    do {
      int16_t wcx = wc.x >> fix_shift;
      int16_t wcy = wc.y >> fix_shift;

      int32_t toff = offset(wcx >> 3, wcy >> 3);

      if (toff != -1 && tiles[toff] != empty_tile_id) {
        uint8_t tile_id = tiles[toff];
        uint8_t transform = transforms ? transforms[toff] : 0;

        // coordinate within sprite
        int u = wcx & 0b111;
        int v = wcy & 0b111;

        // if this tile has a transform then modify the uv coordinates
        if (transform) {
          v = (transform & 0b010) ? (7 - v) : v;
          u = (transform & 0b100) ? (7 - u) : u;
          if (transform & 0b001) { int tmp = u; u = v; v = tmp; }
        }

        // sprite sheet coordinates for top left corner of sprite
        u += (tile_id & 0b1111) * 8;
        v += (tile_id >> 4) * 8;

        // draw as many pixels as possible
        int count = 0;

        do {
          wc += dwc;
          c--;
          count++;
        } while(c && (wc.x >> fix_shift) == wcx && (wc.y >> fix_shift) == wcy);

        auto pen = src->get_pixel({u, v});
        dest->pbf(&pen, dest, doff, count);

        doff += count;

        continue;
      }

      // skip to next tile
      do {
        wc += dwc;
        doff++;
        c--;
      } while(c && (wc.x >> (fix_shift + 3)) == wcx >> 3 && (wc.y >> (fix_shift + 3)) == wcy >> 3);

    } while (c);
  }

}
