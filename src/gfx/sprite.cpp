#include "gfx/sprite.h"

#include <fstream>
#include <sstream>
#include <stdexcept>

namespace pix {

  namespace {

    std::string trim(const std::string& s) {
      size_t start = s.find_first_not_of(" \t\r");
      if (start == std::string::npos)
        return "";
      size_t end = s.find_last_not_of(" \t\r");
      return s.substr(start, end - start + 1);
    }

    [[noreturn]] void fail(const std::string &path, int lineNo, const std::string &msg)
    {
      throw std::runtime_error(path + ":" + std::to_string(lineNo) + ": " + msg);
    }

  } // namespace

  void drawImage(Framebuffer &fb, const Image &img, int x, int y, bool flipX)
  {
    for (int r = 0; r < img.height; ++r)
    {
      const Color *row = img.pixels.data() + r * img.width;
      for (int c = 0; c < img.width; ++c)
      {
        Color px = row[flipX ? img.width - 1 - c : c];
        if ((px >> 24) == 0)
          continue; // fully transparent
        fb.setPixel(x + c, y + r, px);
      }
    }
  }

  const std::vector<Image> *Sprite::anim(const std::string &name) const
  {
    auto it = anims.find(name);
    return it == anims.end() ? nullptr : &it->second;
  }

  void drawSprite(Framebuffer& fb, const Sprite &s, const Image& frame, int x, int y, bool flipX)
  {
    // When mirrored, the origin moves to the opposite column
    int left = flipX ? x - (s.width - 1 - s.originX) : x - s.originX;
    drawImage(fb, frame, left, y - s.originY, flipX);
  }

  Sprite loadSprite(const std::string &path)
  {
    std::ifstream in(path);
    if (!in)
      throw std::runtime_error("Could not open sprite file: " + path);

    enum class Section
    {
      Header,
      Palette,
      Anim
    };
    Section section = Section::Header;

    Sprite sprite;
    std::map<char, Color> palette;
    palette['.'] = 0; // '.' is always transparent

    std::string animName;
    std::vector<std::string> rows; // rows of the frame being read
    std::vector<int> rowLines;     // which line each row came from, for errors
    std::string raw;
    int lineNo = 0;

    while (std::getline(in, raw))
    {
      ++lineNo;
      std::string line = trim(raw);
      if (line.empty() || line.rfind("//", 0) == 0)
        continue;

      if (line == "palette:")
      {
        section = Section::Palette;
        continue;
      }

      if (line.rfind("anim:", 0) == 0)
      {
        if (sprite.width <= 0 || sprite.height <= 0)
          fail(path, lineNo, "'size' must come before the first animation");
        if (!rows.empty())
          fail(path, lineNo, "animation '" + animName + "' ends in the middle of a frame");

        animName = trim(line.substr(5));
        if (animName.empty())
          fail(path, lineNo, "animation needs a name");
        if (sprite.anims.count(animName))
          fail(path, lineNo, "animation '" + animName + "' is defined twice");

        sprite.anims[animName]; // create it
        section = Section::Anim;
        continue;
      }

      switch (section)
      {
      case Section::Header:
      {
        size_t colon = line.find(':');
        if (colon == std::string::npos)
          fail(path, lineNo, "expected 'key: value'");

        std::string key = trim(line.substr(0, colon));
        std::istringstream value(line.substr(colon + 1));

        if (key == "size")
        {
          if (!(value >> sprite.width >> sprite.height) || sprite.width <= 0 || sprite.height <= 0)
            fail(path, lineNo, "expected a size like '8 14'");
        }
        else if (key == "origin")
        {
          if (!(value >> sprite.originX >> sprite.originY))
            fail(path, lineNo, "expected a pixel position like '4 13'");
        }
        else
        {
          fail(path, lineNo, "unknown key '" + key + "'");
        }
        break;
      }

      case Section::Palette:
      {
        std::istringstream entry(line);
        char c;
        int r, g, b, a = 255;
        if (!(entry >> c >> r >> g >> b))
          fail(path, lineNo, "expected a palette entry like 'b 60 120 200'");
        entry >> a; // alpha is optional

        if (c == '.')
          fail(path, lineNo, "'.' is reserved for transparent pixels");
        if (r < 0 || r > 255 || g < 0 || g > 255 || b < 0 || b > 255 || a < 0 || a > 255)
          fail(path, lineNo, "color values must be between 0 and 255");

        palette[c] = rgba(r, g, b, a);
        break;
      }

      case Section::Anim:
      {
        if (static_cast<int>(line.size()) > sprite.width)
          fail(path, lineNo, "row is wider than the sprite's size");

        rows.push_back(line);
        rowLines.push_back(lineNo);

        // A full frame has been read: turn the characters into pixels
        if (static_cast<int>(rows.size()) == sprite.height)
        {
          Image img;
          img.width = sprite.width;
          img.height = sprite.height;
          img.pixels.assign(sprite.width * sprite.height, 0);

          for (int y = 0; y < sprite.height; ++y)
          {
            for (int x = 0; x < static_cast<int>(rows[y].size()); ++x)
            {
              auto it = palette.find(rows[y][x]);
              if (it == palette.end())
                fail(path, rowLines[y], std::string("unknown color '") + rows[y][x] + "'");
              img.pixels[y * sprite.width + x] = it->second;
            }
          }

          sprite.anims[animName].push_back(img);
          rows.clear();
          rowLines.clear();
        }
        break;
      }
      }
    }

    if (!rows.empty())
      throw std::runtime_error(path + ": animation '" + animName + "' ends in the middle of a frame");
    if (sprite.anims.empty())
      throw std::runtime_error(path + ": sprite has no animations");

    return sprite;
  }

} // namespace pix
