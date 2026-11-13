#pragma once
#include "gfx/window.h"
#include "play/state.h"

#include <map>
#include <string>
#include <vector>

// Dialogue and the characters you talk to

// Number of letters on a page once it's wrapped into lines
int pageLength(const std::string &page);

// Shows a list of speeches, splitting them into pages once names and flags are filled in
void startDialogue(DialogueState &d, const std::string &speaker, const std::vector<std::string> &speeches,
                   const std::string &name, const std::map<std::string, int> &flags);

void updateDialogue(const pix::Window &window, DialogueState &d);

// Which way someone at (fx, fy) faces to look at (tx, ty), as seen on screen
Facing facingToward(float fx, float fy, float tx, float ty);

// Is this character on the level right now?
bool npcPresent(const Game &g, const game::NpcSpawn &spawn, int visit);

// Rebuilds the list of characters on the current level, so someone whose
// flag just changed appears or disappears
void refreshNpcs(Game &g);

// The closest character within talking range, or -1
int npcInReach(const Game &g);

// Starts a conversation: the character's scene if they have one,
// otherwise their dialogue file
void talkTo(Game &g, NpcState &npc);
