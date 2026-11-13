#include "play/talk.h"

#include "game/dialogue.h"
#include "gfx/iso.h"
#include "play/scenes.h"
#include "play/text.h"

#include <cmath>

int pageLength(const std::string &page)
{
  int n = 0;
  for (const std::string &line : game::wrapText(page, DIALOGUE_CHARS))
    n += static_cast<int>(line.size());
  return n;
}

void startDialogue(DialogueState &d, const std::string &speaker, const std::vector<std::string> &speeches,
                   const std::string &name, const std::map<std::string, int> &flags)
{
  d.pages.clear();
  for (const std::string &speech : speeches)
  {
    for (const std::string &page : game::paginate(fillText(speech, name, flags), DIALOGUE_CHARS, DIALOGUE_LINES))
      d.pages.push_back(page);
  }
  if (d.pages.empty())
    return;

  d.active = true;
  d.speaker = fillText(speaker, name, flags);
  d.page = 0;
  d.shown = 0;
  d.tick = 0;
}

void updateDialogue(const pix::Window &window, DialogueState &d)
{
  using pix::Key;
  ++d.tick;

  int length = pageLength(d.pages[d.page]);
  if (d.shown < length)
    ++d.shown; // typewriter: one letter per frame

  bool next = window.wasKeyPressed(Key::E) || window.wasKeyPressed(Key::Enter) || window.wasKeyPressed(Key::Space);
  if (!next)
    return;

  // The first press finishes the page; the next one turns it
  if (d.shown < length)
  {
    d.shown = length;
    return;
  }

  ++d.page;
  d.shown = 0;
  if (d.page >= static_cast<int>(d.pages.size()))
    d.active = false;
}

Facing facingToward(float fx, float fy, float tx, float ty)
{
  float dx = tx - fx;
  float dy = ty - fy;
  float sx = (dx - dy) * pix::TILE_W / 2;
  float sy = (dx + dy) * pix::TILE_H / 2;

  if (std::fabs(sx) > std::fabs(sy))
    return sx > 0 ? Facing::Right : Facing::Left;
  return sy > 0 ? Facing::Down : Facing::Up;
}

bool npcPresent(const Game &g, const game::NpcSpawn &spawn, int visit)
{
  if (spawn.firstVisitOnly && visit > 1)
    return false;
  if (!spawn.ifFlag.empty() && flagValue(g, spawn.ifFlag) == 0)
    return false;
  if (!spawn.unlessFlag.empty() && flagValue(g, spawn.unlessFlag) != 0)
    return false;
  return true;
}

void refreshNpcs(Game &g)
{
  const game::Level &level = g.world.levels[g.player.floor];
  int visit = g.visits[g.player.floor];

  std::vector<NpcState> next;
  for (const game::NpcSpawn &spawn : level.npcs)
  {
    if (!npcPresent(g, spawn, visit))
      continue;

    // Characters who stay keep facing the way they were
    NpcState state{&spawn, spawn.x + 0.5f, spawn.y + 0.5f};
    for (const NpcState &old : g.npcs)
    {
      if (old.spawn == &spawn)
        state.facing = old.facing;
    }
    next.push_back(state);
  }
  g.npcs = next;
}

int npcInReach(const Game &g)
{
  int best = -1;
  float bestDist = TALK_RANGE * TALK_RANGE;

  for (int i = 0; i < static_cast<int>(g.npcs.size()); ++i)
  {
    float dx = g.npcs[i].x - g.player.x;
    float dy = g.npcs[i].y - g.player.y;
    float dist = dx * dx + dy * dy;
    if (dist <= bestDist)
    {
      best = i;
      bestDist = dist;
    }
  }
  return best;
}

void talkTo(Game &g, NpcState &npc)
{
  const std::string &name = npc.spawn->name;

  // Face each other
  npc.facing = facingToward(npc.x, npc.y, g.player.x, g.player.y);
  g.player.facing = facingToward(g.player.x, g.player.y, npc.x, npc.y);

  if (g.scenes.count(name))
  {
    ++g.talks[name];
    startScene(g, name);
    return;
  }

  auto it = g.dialogues.find(name);
  if (it == g.dialogues.end())
    return;

  // Choose what they say by whether you've met and whether you carry Burden
  int &timesTalked = g.talks[name];
  const game::DialogueSection *section = it->second.choose(timesTalked == 0, g.burden > 0);
  if (!section || section->lines.empty())
    return;

  ++timesTalked;
  startDialogue(g.dialogue, it->second.speaker, section->lines, g.playerName, g.flags);
}
