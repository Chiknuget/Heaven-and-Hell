#pragma once
#include "gfx/framebuffer.h"

namespace pix
{

  constexpr int TILE_W = 32;
  constexpr int TILE_H = 16;

  struct Point
  {
    int x;
    int y;
  };

  // Converts tile coordinates to the top-left corner of the tile's
  // bounding box on screen (before any camera offset is added).
  constexpr Point isoToScreen(int tx, int ty)
  {
    return {(tx - ty) * (TILE_W / 2), (tx + ty) * (TILE_H / 2)};
  }

  // A flat diamond whose bounding box starts at (sx, sy).
  void drawIsoTile(Framebuffer &fb, int sx, int sy, Color top);

  // A block standing on the diamond at (sx, sy), raised by 'height' pixels,
  // with a shaded left and right face.
  void drawIsoBlock(Framebuffer &fb, int sx, int sy, int height, Color top, Color left, Color right);

} // namespace pix
