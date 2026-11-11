#include "gfx/iso.h"
#include <cstdlib>

namespace pix
{

  namespace
  {

    // How many pixels row r of the diamond extends to each side of its center.
    // Gives row widths 2, 6, 10 ... 30, 30 ... 6, 2, which tile perfectly
    // with no gaps or overlapping pixels between neighbors.
    int rowHalfWidth(int r)
    {
      return TILE_W / 2 - (TILE_W / TILE_H) * std::abs(2 * r + 1 - TILE_H) / 2;
    }

  } // namespace

  void drawIsoTile(Framebuffer &fb, int sx, int sy, Color top)
  {
    int cx = sx + TILE_W / 2;
    for (int r = 0; r < TILE_H; ++r)
    {
      int hw = rowHalfWidth(r);
      fb.fillRect(cx - hw, sy + r, hw * 2, 1, top);
    }
  }

  void drawIsoBlock(Framebuffer &fb, int sx, int sy, int height, Color top, Color left, Color right)
  {
    int cx = sx + TILE_W / 2;

    // Build the sides by stacking the bottom half of the diamond
    // one pixel at a time, from the ground up to the top face.
    for (int i = 0; i < height; ++i)
    {
      for (int r = TILE_H / 2; r < TILE_H; ++r)
      {
        int hw = rowHalfWidth(r);
        int y = sy - i + r;
        fb.fillRect(cx - hw, y, hw, 1, left);
        fb.fillRect(cx, y, hw, 1, right);
      }
    }

    drawIsoTile(fb, sx, sy - height, top);
  }

} // namespace pix
