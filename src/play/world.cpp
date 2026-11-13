#include "play/world.h"

#include "gfx/iso.h"
#include "gfx/sprite.h"
#include "play/missions.h"
#include "play/movement.h"
#include "play/save.h"
#include "play/scenes.h"
#include "play/talk.h"

#include <cmath>
#include <cstdio>
#include <exception>
#include <fstream>
#include <stdexcept>

namespace
{

  bool fileExists(const std::string &path)
  {
    return std::ifstream(path).good();
  }

} // namespace

Destination spawnOf(const game::World &w, int floor)
{
  const game::Level &l = w.levels[floor];
  return Destination{floor, l.spawnX + 0.5f, l.spawnY + 0.5f};
}

Destination deathDestination(const Game &g)
{
  return (g.world.purgatoryBottom >= 0) ? spawnOf(g.world, g.world.purgatoryBottom) : g.checkpoint;
}

ExitInfo exitUnderPlayer(const Game &g)
{
  ExitInfo info;
  const game::World &w = g.world;
  const Player &p = g.player;
  const game::Level &here = w.levels[p.floor];
  int tx = static_cast<int>(std::floor(p.x));
  int ty = static_cast<int>(std::floor(p.y));
  char c = here.at(tx, ty);

  if (c == game::TILE_GATE)
  {
    info.valid = true;
    info.usable = true;
    info.text = "return";
    info.dest = g.checkpoint;
    return info;
  }

  if (c == game::TILE_TOLL)
  {
    info.valid = true;
    info.usable = true;
    info.text = "return (+1 Burden)";
    info.dest = g.checkpoint;
    info.burdenCost = 1;
    return info;
  }

  if (c != game::TILE_UP && c != game::TILE_DOWN)
    return info;

  bool up = (c == game::TILE_UP);
  int target = up ? p.floor - 1 : p.floor + 1;
  if (target < 0 || target >= static_cast<int>(w.levels.size()) || !w.sameStack(p.floor, target))
    return info;

  char arriveOn = up ? game::TILE_DOWN : game::TILE_UP;
  int ax = 0, ay = 0;
  if (!w.levels[target].findNth(arriveOn, here.connectorIndex(tx, ty), ax, ay))
    return info;

  info.valid = true;
  info.dest = Destination{target, ax + 0.5f, ay + 0.5f};

  // The burdened cannot enter Heaven
  if (up && target == game::HEAVEN && g.burden > 0)
  {
    info.text = "Your burden is too heavy to ascend";
    return info;
  }

  info.usable = true;
  info.text = up ? "ascend" : "descend";
  return info;
}

void startTransition(Game &g, const Destination &dest, int length, pix::Color color)
{
  Transition &t = g.transition;
  t.phase = FadePhase::FadeOut;
  t.frame = 0;
  t.length = length;
  t.color = color;
  t.dest = dest;
}

int fadeAmount(const Transition &t)
{
  switch (t.phase)
  {
  case FadePhase::FadeOut:
    return t.frame * 255 / t.length;
  case FadePhase::FadeIn:
    return 255 - t.frame * 255 / t.length;
  default:
    return 0;
  }
}

void setTile(Game &g, int floor, int x, int y, char tile)
{
  if (floor < 0 || floor >= static_cast<int>(g.world.levels.size()))
    return;

  game::Level &level = g.world.levels[floor];
  if (x < 0 || y < 0 || y >= level.height())
    return;

  std::string &row = level.rows[y];
  if (static_cast<int>(row.size()) <= x)
    row.resize(x + 1, game::TILE_VOID);
  row[x] = tile;

  g.tileChanges.push_back(TileChange{floor, x, y, tile});
}

void arrive(Game &g, const Destination &d)
{
  const game::World &w = g.world;
  g.player.floor = d.floor; // facing is kept, so you arrive looking the same way
  g.player.x = d.x;
  g.player.y = d.y;
  g.effects = Effects{};    // whatever a scene left on screen stays behind
  g.pendingAutosave = true; // saved once any arrival scene or conversation is over

  int visit = ++g.visits[d.floor];

  const game::Level &level = w.levels[d.floor];
  showBanner(g.banner, level.name, level.subtitle);

  if (!w.inPurgatory(d.floor))
    g.checkpoint = d; // arriving on a main floor saves your place

  if (d.floor == w.purgatoryTop)
    g.burden = 0; // the summit lifts every burden

  refreshNpcs(g);

  // The level's own scene, if it has one...
  if (!level.enterScene.empty() && (!level.enterOnce || visit == 1))
  {
    startScene(g, level.enterScene);
    return;
  }

  // ...otherwise anyone who greets you starts talking right away
  for (NpcState &npc : g.npcs)
  {
    if (npc.spawn->greet)
    {
      talkTo(g, npc);
      break;
    }
  }
}

void loadAssets(Game &g)
{
  // Everything is loaded fresh, so edits to levels, dialogue, sprites and scenes show up
  g.world = game::loadWorld("levels/world.txt");
  g.sprites["traveler"] = pix::loadSprite("sprites/traveler.txt");

  auto loadSceneOnce = [&](const std::string &sceneName)
  {
    if (!g.scenes.count(sceneName))
      g.scenes[sceneName] = game::loadScene("scenes/" + sceneName + ".txt");
  };

  loadSceneOnce("prologue");

  // Load everything any level uses, so a missing or broken file is
  // reported now rather than when you first get there
  for (const game::Level &level : g.world.levels)
  {
    if (!level.enterScene.empty())
      loadSceneOnce(level.enterScene);

    for (const game::NpcSpawn &npc : level.npcs)
    {
      if (!g.sprites.count(npc.sprite))
        g.sprites[npc.sprite] = pix::loadSprite("sprites/" + npc.sprite + ".txt");

      // A character with a scene file uses it; otherwise their dialogue file
      if (fileExists("scenes/" + npc.name + ".txt"))
      {
        loadSceneOnce(npc.name);
      }
      else if (!g.dialogues.count(npc.name))
      {
        if (!fileExists("dialogue/" + npc.name + ".txt"))
          throw std::runtime_error("Character '" + npc.name + "' needs scenes/" + npc.name +
                                   ".txt or dialogue/" + npc.name + ".txt");
        g.dialogues[npc.name] = game::loadDialogue("dialogue/" + npc.name + ".txt");
      }
    }
  }
}

void startNewGame(Game &g, const std::string &name)
{
  g = Game{};
  g.playerName = name;
  loadAssets(g);

  arrive(g, spawnOf(g.world, game::HEAVEN));
  g.banner.frame = BANNER_FRAMES; // the prologue shows the banner itself, at the end
  startScene(g, "prologue");
  g.inProgress = true;
}

void updateGame(const pix::Window &window, Game &g)
{
  using pix::Key;
  Transition &t = g.transition;
  Player &pl = g.player;

  pl.walking = false; // set again below if the player actually moves

  // The mission log pauses everything while it's open
  if (window.wasKeyPressed(Key::Tab))
    g.logOpen = !g.logOpen;
  if (g.logOpen)
    return;

  if (g.banner.frame < BANNER_FRAMES)
    ++g.banner.frame;
  if (g.toast.frame < TOAST_FRAMES)
    ++g.toast.frame;

  // While fading out the player is frozen. Once the screen is fully
  // covered, move them to the destination and start fading back in.
  if (t.phase == FadePhase::FadeOut)
  {
    if (++t.frame >= t.length)
    {
      arrive(g, t.dest);
      t.phase = FadePhase::FadeIn;
      t.frame = 0;
    }
    return;
  }

  if (t.phase == FadePhase::FadeIn && ++t.frame >= t.length)
    t.phase = FadePhase::None;

  // Someone is talking: the player waits (and the text waits for the fade)
  if (g.dialogue.active)
  {
    if (t.phase == FadePhase::None)
      updateDialogue(window, g.dialogue);
    return;
  }

  // A scene is in charge: it decides whether the player can walk this frame
  if (g.scene.active())
  {
    if (t.phase != FadePhase::None || !updateScene(window, g))
      return;
  }

  const game::Level &level = g.world.levels[pl.floor];

  int ix = (window.isKeyDown(Key::D) || window.isKeyDown(Key::Right)) - (window.isKeyDown(Key::A) || window.isKeyDown(Key::Left));
  int iy = (window.isKeyDown(Key::S) || window.isKeyDown(Key::Down)) - (window.isKeyDown(Key::W) || window.isKeyDown(Key::Up));

  // Facing and walk animation. Diagonals use the side view.
  if (ix != 0 || iy != 0)
  {
    pl.walking = true;
    ++pl.animTick;

    if (ix > 0)
      pl.facing = Facing::Right;
    else if (ix < 0)
      pl.facing = Facing::Left;
    else if (iy < 0)
      pl.facing = Facing::Up;
    else
      pl.facing = Facing::Down;
  }
  else
  {
    pl.animTick = 0;
  }

  // Velocity in screen pixels per frame
  float vx = ix * SPEED_X;
  float vy = iy * SPEED_X * 0.5f;

  // Screen pixels -> world tiles
  float dx = vx / pix::TILE_W + vy / pix::TILE_H;
  float dy = vy / pix::TILE_H - vx / pix::TILE_W;

  if (dx != 0.0f || dy != 0.0f)
  {
    moveWithCollision(level, g.npcs, pl.x, pl.y, dx, dy);
  }

  // Nothing else happens until the current transition has finished,
  // and no talking, stairs or dying in the middle of a scene
  if (t.phase != FadePhase::None || g.scene.active())
    return;

  // Save automatically once things have settled after arriving somewhere
  if (g.pendingAutosave)
  {
    g.pendingAutosave = false;
    try
    {
      saveGame(g);
    }
    catch (const std::exception &e)
    {
      std::fprintf(stderr, "Autosave failed: %s\n", e.what());
      showToast(g, "Could not save the game");
    }
  }

  // Death: the middle of the player is over something deadly
  char under = level.at(static_cast<int>(std::floor(pl.x)), static_cast<int>(std::floor(pl.y)));
  if (game::tileType(under).deadly)
  {
    startTransition(g, deathDestination(g), DEATH_FADE, pix::rgb(255, 255, 255));
    return;
  }

  if (!window.wasKeyPressed(Key::E))
    return;

  // E: talking takes priority over stairs and gates
  int npc = npcInReach(g);
  if (npc >= 0)
  {
    talkTo(g, g.npcs[npc]);
    return;
  }

  ExitInfo ex = exitUnderPlayer(g);
  if (ex.valid && ex.usable)
  {
    g.burden += ex.burdenCost;
    startTransition(g, ex.dest, STAIRS_FADE, pix::rgb(0, 0, 0));
  }
}
