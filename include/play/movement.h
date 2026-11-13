#pragma once
#include "play/state.h"

#include <vector>

// Does a player-sized box centered at (x, y) touch any blocked tile?
bool boxBlocked(const game::Level &level, float x, float y);

// Would the player at (x, y) overlap any character?
bool npcBlocked(const std::vector<NpcState> &npcs, float x, float y);

// Moves one axis at a time. If an axis runs into a wall, the player is
// pushed right up against it, which lets them slide along it. Characters
// simply stop movement into them.
void moveWithCollision(const game::Level &level, const std::vector<NpcState> &npcs,
                       float &px, float &py, float dx, float dy);
