#pragma once
#include "gfx/framebuffer.h"
#include "play/state.h"

#include <vector>

// Draws the level's tiles and the actors standing on it, with the camera on the player
void drawLevel(pix::Framebuffer &fb, const game::Level &level, const Player &p, std::vector<Actor> actors);

// Draws a whole frame of the game: the world, then everything on top of it
void drawGame(pix::Framebuffer &fb, const Game &g);
