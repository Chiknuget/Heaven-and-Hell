#pragma once
#include "gfx/framebuffer.h"
#include <map>
#include <string>
#include <vector>

namespace pix
{

  struct Image
  {
    int width = 0;
    int height = 0;
    std::vector<Color> pixels; // row by row; alpha 0 = transparent
  };

  // Draws an image with its top-left corner at (x, y), optionally mirroring it. Transparent pixels are skipped.
  void drawImage(Framebuffer &fb, const Image &img, int x, int y, bool flipX = false);

  struct Sprite
  {
    int width = 0;
    int height = 0;
    int originX = 0; // the pixel that sits on the ground (or the bottom of the sprite)
    int originY = 0;
    std::map<std::string, std::vector<Image>> anims; // name -> frames

    // nullptr if there's no animation with that name
    const std::vector<Image> *anim(const std::string &name) const;
  };

  // Draws a frame so the sprite's origin lands on (x, y).
  // When flipped, the image is mirrored around the origin.
  void drawSprite(Framebuffer &fb, const Sprite &s, const Image &frame, int x, int y, bool flipX = false);

  // Loads a sprite from a text file. Throws std::runtime_error with the
  // file name and line number if something is wrong.
  Sprite loadSprite(const std::string &path);

} // namespace pix
