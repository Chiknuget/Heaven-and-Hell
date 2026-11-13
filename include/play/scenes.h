#pragma once
#include "gfx/window.h"
#include "play/state.h"

#include <string>

// Starts the scene with this name, if it's loaded
void startScene(Game &g, const std::string &name);

// Runs the current scene for one frame. Returns true if the player may
// walk this frame (during "explore", or once the scene has ended).
bool updateScene(const pix::Window &window, Game &g);
