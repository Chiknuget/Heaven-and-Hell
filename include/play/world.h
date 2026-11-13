#pragma once
#include "gfx/window.h"
#include "play/state.h"

#include <string>

// Moving between places, and the game's frame-by-frame update

Destination spawnOf(const game::World &w, int floor);

// Where the dead wake up: Purgatory if there is one, otherwise the checkpoint
Destination deathDestination(const Game &g);

// The Nth '^' on a level leads to the Nth 'v' on the level above, and vice
// versa, within the same stack. Gates lead back to the checkpoint.
ExitInfo exitUnderPlayer(const Game &g);

void startTransition(Game &g, const Destination &dest, int length, pix::Color color);

// How strongly a transition's color covers the screen: 0 = not at all, 255 = fully
int fadeAmount(const Transition &t);

// Puts the player at a destination and handles everything that happens on arrival
void arrive(Game &g, const Destination &d);

// Loads the world and every sprite, scene and dialogue file it uses
void loadAssets(Game &g);

// Changes a tile on a level, and remembers the change for saving
void setTile(Game &g, int floor, int x, int y, char tile);

// Loads everything and starts the prologue
void startNewGame(Game &g, const std::string &name);

void updateGame(const pix::Window &window, Game &g);
