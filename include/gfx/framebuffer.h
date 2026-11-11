#pragma once
#include <cstdint>
#include <vector>

namespace pix
{

  using Color = std::uint32_t; // 0xAARRGGBB

  // Alpha (a) is opacity: 255 is solid, 0 is invisible, anything in between
  // blends with whatever is already there.
  constexpr Color rgba(std::uint8_t r, std::uint8_t g, std::uint8_t b, std::uint8_t a)
  {
    return (Color(a) << 24) | (Color(r) << 16) | (Color(g) << 8) | Color(b);
  }

  constexpr Color rgb(std::uint8_t r, std::uint8_t g, std::uint8_t b)
  {
    return rgba(r, g, b, 255);
  }

  class Framebuffer
  {
  public:
    Framebuffer(int width, int height);

    int width() const { return m_width; }
    int height() const { return m_height; }

    void clear(Color c);
    void setPixel(int x, int y, Color c);
    void fillRect(int x, int y, int w, int h, Color c);

    const Color *data() const { return m_pixels.data(); }

  private:
    int m_width;
    int m_height;
    std::vector<Color> m_pixels;
  };

} // namespace pix
