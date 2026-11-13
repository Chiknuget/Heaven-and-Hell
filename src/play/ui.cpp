#include "play/ui.h"

#include "game/dialogue.h"
#include "gfx/font.h"
#include "play/talk.h"
#include "play/text.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <vector>

namespace
{

  const pix::Color BORDER = pix::rgb(200, 190, 210);
  const pix::Color BACKGROUND = pix::rgb(24, 20, 32);
  const pix::Color TEXT = pix::rgb(240, 240, 240);
  const pix::Color GOLD = pix::rgb(255, 210, 80);
  const pix::Color GREY = pix::rgb(130, 125, 140);

  // Opacity for something that fades in, holds, then fades out
  int fadeInOut(int frame, int total, int fadeLength)
  {
    if (frame < fadeLength)
      return frame * 255 / fadeLength;
    if (frame > total - fadeLength)
      return (total - frame) * 255 / fadeLength;
    return 255;
  }

  // A small box in the bottom-left corner, with the middle part highlighted
  void drawPromptBox(pix::Framebuffer &fb, const std::string &before,
                     const std::string &highlight, const std::string &after)
  {
    const int pad = 4;
    int boxW = pix::textWidth(before + highlight + after) + pad * 2;
    int boxH = pix::GLYPH_H + pad * 2 - 1;
    int boxX = 4;
    int boxY = GAME_H - 4 - boxH;

    fb.fillRect(boxX, boxY, boxW, boxH, BORDER);
    fb.fillRect(boxX + 1, boxY + 1, boxW - 2, boxH - 2, BACKGROUND);

    int x = boxX + pad;
    int y = boxY + pad;
    x = pix::drawText(fb, x, y, before, TEXT);
    x = pix::drawText(fb, x, y, highlight, GOLD);
    pix::drawText(fb, x, y, after, TEXT);
  }

} // namespace

pix::Color mix(pix::Color a, pix::Color b, int t, int max)
{
  auto channel = [&](int shift)
  {
    int ca = (a >> shift) & 0xFF;
    int cb = (b >> shift) & 0xFF;
    return static_cast<std::uint8_t>(ca + (cb - ca) * t / max);
  };
  return pix::rgb(channel(16), channel(8), channel(0));
}

void drawGradient(pix::Framebuffer &fb, pix::Color top, pix::Color bottom)
{
  for (int y = 0; y < GAME_H; ++y)
  {
    fb.fillRect(0, y, GAME_W, 1, mix(top, bottom, y, GAME_H - 1));
  }
}

int drawShadowedText(pix::Framebuffer &fb, int x, int y, const std::string &text,
                     pix::Color color, pix::Color shadow, int scale)
{
  pix::drawText(fb, x + scale, y + scale, text, shadow, scale);
  return pix::drawText(fb, x, y, text, color, scale);
}

void drawPrompt(pix::Framebuffer &fb, const std::string &verb)
{
  drawPromptBox(fb, "Press ", "E", " to " + verb);
}

void drawNotice(pix::Framebuffer &fb, const std::string &text)
{
  drawPromptBox(fb, text, "", "");
}

void drawBurden(pix::Framebuffer &fb, int burden)
{
  std::string text = "Burden: " + std::to_string(burden);
  int w = pix::textWidth(text) + 8;
  int h = pix::GLYPH_H + 7;
  int x = GAME_W - 4 - w;
  int y = 4;

  fb.fillRect(x, y, w, h, pix::rgb(150, 60, 70));
  fb.fillRect(x + 1, y + 1, w - 2, h - 2, pix::rgb(30, 14, 18));
  pix::drawText(fb, x + 4, y + 4, text, pix::rgb(230, 170, 170));
}

void drawBanner(pix::Framebuffer &fb, const Banner &b)
{
  if (b.frame >= BANNER_FRAMES)
    return;

  int a = fadeInOut(b.frame, BANNER_FRAMES, BANNER_FADE);

  const int scale = 2;
  const int bandY = 20;
  const bool hasSubtitle = !b.subtitle.empty();
  const int bandH = pix::GLYPH_H * scale + 12 + (hasSubtitle ? 11 : 0);

  // See-through dark band with thin gold lines above and below
  fb.fillRect(0, bandY, GAME_W, bandH, pix::rgba(0, 0, 0, a * 140 / 255));
  fb.fillRect(0, bandY, GAME_W, 1, pix::rgba(255, 220, 150, a * 180 / 255));
  fb.fillRect(0, bandY + bandH - 1, GAME_W, 1, pix::rgba(255, 220, 150, a * 180 / 255));

  int titleX = (GAME_W - pix::textWidth(b.title, scale)) / 2;
  int titleY = bandY + 7;
  drawShadowedText(fb, titleX, titleY, b.title, pix::rgba(255, 240, 210, a), pix::rgba(0, 0, 0, a), scale);

  if (hasSubtitle)
  {
    int subX = (GAME_W - pix::textWidth(b.subtitle)) / 2;
    int subY = titleY + pix::GLYPH_H * scale + 3;
    drawShadowedText(fb, subX, subY, b.subtitle, pix::rgba(220, 200, 170, a), pix::rgba(0, 0, 0, a));
  }
}

void drawToast(pix::Framebuffer &fb, const Toast &t)
{
  if (t.frame >= TOAST_FRAMES)
    return;

  int a = fadeInOut(t.frame, TOAST_FRAMES, TOAST_FADE);
  int w = pix::textWidth(t.text) + 8;
  int h = pix::GLYPH_H + 7;

  fb.fillRect(4, 4, w, h, pix::rgba(255, 210, 80, a * 200 / 255));
  fb.fillRect(5, 5, w - 2, h - 2, pix::rgba(24, 20, 32, a * 230 / 255));
  pix::drawText(fb, 8, 8, t.text, pix::rgba(255, 230, 180, a));
}

void drawMissionLog(pix::Framebuffer &fb, const Game &g)
{
  fb.fillRect(0, 0, GAME_W, GAME_H, pix::rgba(0, 0, 0, 150)); // dim the game behind it

  const int boxX = 20;
  const int boxY = 14;
  const int boxW = GAME_W - 40;
  const int boxH = GAME_H - 28;
  fb.fillRect(boxX, boxY, boxW, boxH, BORDER);
  fb.fillRect(boxX + 1, boxY + 1, boxW - 2, boxH - 2, BACKGROUND);

  const std::string title = "Missions";
  drawShadowedText(fb, (GAME_W - pix::textWidth(title, 2)) / 2, boxY + 8, title, GOLD, pix::rgb(0, 0, 0), 2);

  int y = boxY + 32;
  if (g.missions.empty())
    pix::drawText(fb, boxX + 10, y, "No missions yet.", GREY);

  // Newest first
  for (auto it = g.missions.rbegin(); it != g.missions.rend(); ++it)
  {
    const Mission &m = *it;
    if (y > boxY + boxH - 24)
      break; // out of room

    pix::drawText(fb, boxX + 10, y, m.done ? m.title + " (complete)" : m.title, m.done ? GREY : GOLD);
    y += 11;

    if (!m.done && !m.objective.empty())
    {
      for (const std::string &line : game::wrapText(fillText(m.objective, g.playerName, g.flags), 42))
      {
        pix::drawText(fb, boxX + 16, y, line, TEXT);
        y += 10;
      }
    }
    y += 6;
  }

  const std::string hint = "Tab: close";
  pix::drawText(fb, (GAME_W - pix::textWidth(hint)) / 2, boxY + boxH - 13, hint, GREY);
}

void drawDialogue(pix::Framebuffer &fb, const DialogueState &d)
{
  if (!d.active)
    return;

  const int pad = 6;
  const int lineH = 10;
  const int boxX = 8;
  const int boxW = GAME_W - 16;
  const int boxH = DIALOGUE_LINES * lineH - 2 + pad * 2;
  const int boxY = GAME_H - 6 - boxH;

  fb.fillRect(boxX, boxY, boxW, boxH, BORDER);
  fb.fillRect(boxX + 1, boxY + 1, boxW - 2, boxH - 2, BACKGROUND);

  // Name tab on top of the box
  if (!d.speaker.empty())
  {
    int tabW = pix::textWidth(d.speaker) + 8;
    fb.fillRect(boxX, boxY - 11, tabW, 12, BORDER);
    fb.fillRect(boxX + 1, boxY - 10, tabW - 2, 11, BACKGROUND); // joins the tab to the box
    pix::drawText(fb, boxX + 4, boxY - 8, d.speaker, GOLD);
  }

  // Text, revealed letter by letter
  const std::string &page = d.pages[d.page];
  std::vector<std::string> lines = game::wrapText(page, DIALOGUE_CHARS);
  int remaining = d.shown;

  for (int i = 0; i < static_cast<int>(lines.size()) && remaining > 0; ++i)
  {
    int count = std::min(remaining, static_cast<int>(lines[i].size()));
    pix::drawText(fb, boxX + pad, boxY + pad + i * lineH, lines[i].substr(0, count), TEXT);
    remaining -= static_cast<int>(lines[i].size());
  }

  // Blinking arrow once the page is fully shown
  if (d.shown >= pageLength(page) && (d.tick / 20) % 2 == 0)
  {
    pix::drawText(fb, boxX + boxW - pad - pix::GLYPH_W, boxY + boxH - pad - 6, "v", GOLD);
  }
}

void drawChoice(pix::Framebuffer &fb, const game::SceneCommand &c, int selected)
{
  const int pad = 6;
  const int lineH = 12;
  const int arrowW = pix::GLYPH_ADVANCE * 2;
  const int count = static_cast<int>(c.options.size());

  int widest = 0;
  for (const game::SceneOption &option : c.options)
    widest = std::max(widest, pix::textWidth(option.text));

  int boxW = widest + arrowW + pad * 2;
  int boxH = count * lineH - (lineH - pix::GLYPH_H) + pad * 2;
  int boxX = (GAME_W - boxW) / 2;
  int boxY = (GAME_H - boxH) / 2 - 10;

  fb.fillRect(boxX, boxY, boxW, boxH, BORDER);
  fb.fillRect(boxX + 1, boxY + 1, boxW - 2, boxH - 2, BACKGROUND);

  for (int i = 0; i < count; ++i)
  {
    bool isSelected = (i == selected);
    int y = boxY + pad + i * lineH;
    if (isSelected)
      pix::drawText(fb, boxX + pad, y, ">", GOLD);
    pix::drawText(fb, boxX + pad + arrowW, y, c.options[i].text, isSelected ? GOLD : TEXT);
  }
}

void drawCaption(pix::Framebuffer &fb, const std::string &text, int tick)
{
  std::vector<std::string> lines = game::wrapText(text, 40);
  const int lineH = 12;
  int y = (GAME_H - static_cast<int>(lines.size()) * lineH) / 2;

  for (const std::string &line : lines)
  {
    int x = (GAME_W - pix::textWidth(line)) / 2;
    pix::drawText(fb, x, y, line, pix::rgb(230, 220, 210));
    y += lineH;
  }

  if ((tick / 30) % 2 == 0)
  {
    const std::string hint = "Press E";
    pix::drawText(fb, (GAME_W - pix::textWidth(hint)) / 2, GAME_H - 16, hint, pix::rgb(140, 130, 130));
  }
}

void drawGlow(pix::Framebuffer &fb, int radius)
{
  if (radius <= 0)
    return;

  const int cx = GAME_W / 2;
  const int cy = GAME_H / 2 - 8;
  const float r = static_cast<float>(radius);

  for (int y = std::max(0, cy - radius); y < std::min(GAME_H, cy + radius); ++y)
  {
    for (int x = std::max(0, cx - radius); x < std::min(GAME_W, cx + radius); ++x)
    {
      float dx = static_cast<float>(x - cx);
      float dy = static_cast<float>(y - cy);
      float d = std::sqrt(dx * dx + dy * dy);
      if (d >= r)
        continue;

      int a = static_cast<int>(235.0f * (1.0f - d / r));
      fb.setPixel(x, y, pix::rgba(255, 250, 225, a));
    }
  }
}
