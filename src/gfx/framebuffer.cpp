#include "gfx/framebuffer.h"
#include <algorithm>

namespace pix
{

	namespace
	{

		// Mixes src over dst according to src's alpha. The result is always solid.
		Color blend(Color dst, Color src)
		{
			std::uint32_t a = src >> 24;
			if (a == 255)
				return src;
			if (a == 0)
				return dst;

			std::uint32_t ia = 255 - a;
			std::uint32_t r = (((src >> 16) & 0xFF) * a + ((dst >> 16) & 0xFF) * ia) / 255;
			std::uint32_t g = (((src >> 8) & 0xFF) * a + ((dst >> 8) & 0xFF) * ia) / 255;
			std::uint32_t b = ((src & 0xFF) * a + (dst & 0xFF) * ia) / 255;
			return 0xFF000000u | (r << 16) | (g << 8) | b;
		}

	} // namespace

	Framebuffer::Framebuffer(int width, int height)
			: m_width(width), m_height(height), m_pixels(width * height, rgb(0, 0, 0)) {}

	void Framebuffer::clear(Color c)
	{
		std::fill(m_pixels.begin(), m_pixels.end(), c);
	}

	void Framebuffer::setPixel(int x, int y, Color c)
	{
		if (x < 0 || y < 0 || x >= m_width || y >= m_height)
			return;
		Color &dst = m_pixels[y * m_width + x];
		dst = blend(dst, c);
	}

	void Framebuffer::fillRect(int x, int y, int w, int h, Color c)
	{
		std::uint32_t alpha = c >> 24;
		if (alpha == 0)
			return;

		// Clip the rectangle to the framebuffer bounds
		int x0 = std::max(x, 0);
		int y0 = std::max(y, 0);
		int x1 = std::min(x + w, m_width);
		int y1 = std::min(y + h, m_height);

		// Entirely off-screen (or zero size): nothing to draw
		if (x0 >= x1 || y0 >= y1)
			return;

		for (int py = y0; py < y1; ++py)
		{
			Color *row = m_pixels.data() + py * m_width;
			if (alpha == 255)
			{
				std::fill(row + x0, row + x1, c); // solid: fast path
			}
			else
			{
				for (int px = x0; px < x1; ++px)
					row[px] = blend(row[px], c);
			}
		}
	}

} // namespace pix
