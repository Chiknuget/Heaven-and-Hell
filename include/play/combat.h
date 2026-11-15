#pragma once
#include "gfx/window.h"
#include "play/state.h"

// Puts the current level's enemies in place (called on arrival and after loading)
void spawnEnemies(Game &g);

// One frame of fighting: the player's swing, knockback, and every enemy
void updateCombat(const pix::Window &window, Game &g);
