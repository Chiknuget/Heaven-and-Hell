#pragma once
#include "gfx/framebuffer.h"
#include <string>

namespace pix
{

	constexpr int GLYPH_W = 5;			 // pixels wide
	constexpr int GLYPH_H = 8;			 // 7 rows + 1 row for descenders (g, j, p, q, y)
	constexpr int GLYPH_ADVANCE = 6; // glyph width + 1 pixel of spacing

	// Draws text with its top-left corner at (x, y). Each font pixel becomes a
	// scale x scale block, so scale 2 is twice as big.
	// Returns the x position where the next character would go,
	// so you can draw several pieces in different colors back to back.
	int drawText(Framebuffer &fb, int x, int y, const std::string &text, Color color, int scale = 1);

	// Width in pixels of the text as drawn
	int textWidth(const std::string &text, int scale = 1);

} // namespace pix
