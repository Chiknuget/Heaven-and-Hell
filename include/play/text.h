#pragma once
#include "play/state.h"

#include <map>
#include <string>

// Flags and filling in text

int flagValue(const Game &g, const std::string &flag);

// Fills in {name} with the player's name and {anything} with that flag's value
std::string fillText(const std::string &text, const std::string &name, const std::map<std::string, int> &flags);

// Compares a and b with one of == != < <= > >=
bool compareValues(int a, const std::string &op, int b);
