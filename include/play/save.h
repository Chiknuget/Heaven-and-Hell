#pragma once
#include "play/state.h"

// Saving and loading the game, in saves/save.txt

bool saveExists();

// Saving only works when no scene, conversation or transition is running
bool canSave(const Game &g);

// Throws std::exception if the file can't be written
void saveGame(const Game &g);

// Replaces g with the saved game. Throws std::exception (with the line
// number) if the save is missing or broken, and leaves g untouched.
void loadGame(Game &g);
