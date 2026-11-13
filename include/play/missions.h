#pragma once
#include "play/state.h"

#include <string>

// Banners, notices and the mission log's contents

void showBanner(Banner &b, const std::string &title, const std::string &subtitle);
void showToast(Game &g, const std::string &text);

Mission *findMission(Game &g, const std::string &id);
void startMission(Game &g, const std::string &id, const std::string &title);
void setObjective(Game &g, const std::string &id, const std::string &text);
void completeMission(Game &g, const std::string &id);
