#pragma once
#include "gfx/framebuffer.h"
#include "play/state.h"

#include <string>

// Everything drawn on top of the world: text, boxes, banners and effects

// Blend from color a to color b; t goes from 0 to max
pix::Color mix(pix::Color a, pix::Color b, int t, int max);

void drawGradient(pix::Framebuffer &fb, pix::Color top, pix::Color bottom);

// Text with a drop shadow one font-pixel down and to the right
int drawShadowedText(pix::Framebuffer &fb, int x, int y, const std::string &text,
                     pix::Color color, pix::Color shadow, int scale = 1);

// "Press E to <verb>" in the bottom-left corner
void drawPrompt(pix::Framebuffer &fb, const std::string &verb);

// A message in the same place, with no key to press
void drawNotice(pix::Framebuffer &fb, const std::string &text);

void drawBurden(pix::Framebuffer &fb, int burden);
void drawBanner(pix::Framebuffer &fb, const Banner &b);
void drawToast(pix::Framebuffer &fb, const Toast &t);
void drawMissionLog(pix::Framebuffer &fb, const Game &g);
void drawDialogue(pix::Framebuffer &fb, const DialogueState &d);
void drawChoice(pix::Framebuffer &fb, const game::SceneCommand &c, int selected);

// Centered text on its own, for endings and title cards
void drawCaption(pix::Framebuffer &fb, const std::string &text, int tick);

// A soft, blinding light centered just above the middle of the screen
void drawGlow(pix::Framebuffer &fb, int radius);
