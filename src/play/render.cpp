#include "play/render.h"

#include "gfx/iso.h"
#include "gfx/sprite.h"
#include "play/talk.h"
#include "play/ui.h"
#include "play/world.h"

#include <algorithm>
#include <cmath>

namespace
{

  const char *facingName(Facing f)
  {
    switch (f)
    {
    case Facing::Up:
      return "up";
    case Facing::Left:
      return "left";
    case Facing::Right:
      return "right";
    default:
      return "down";
    }
  }

  // Chooses which frame of a sprite to show. Sets flip when a missing
  // left or right animation is made by mirroring the other one.
  const pix::Image *pickFrame(const pix::Sprite &s, Facing facing, bool walking, int tick, bool &flip)
  {
    flip = false;
    const std::vector<pix::Image> *frames = s.anim(facingName(facing));

    if (!frames && facing == Facing::Left)
    {
      frames = s.anim("right");
      flip = true;
    }
    if (!frames && facing == Facing::Right)
    {
      frames = s.anim("left");
      flip = true;
    }
    if (!frames)
    {
      frames = s.anim("down");
      flip = false;
    }
    if (!frames || frames->empty())
      return nullptr;

    // Frame 0 is standing; the rest loop while walking
    int n = static_cast<int>(frames->size());
    int index = 0;
    if (walking && n > 1)
      index = 1 + (tick / WALK_FRAME_TICKS) % (n - 1);

    return &(*frames)[index];
  }

  // (fx, fy) is where the actor's feet touch the ground
  void drawActor(pix::Framebuffer &fb, const Actor &a, int fx, int fy)
  {
    fb.fillRect(fx - 4, fy - 1, 8, 2, pix::rgba(0, 0, 0, 90)); // soft shadow

    if (!a.sprite)
      return;

    bool flip = false;
    const pix::Image *frame = pickFrame(*a.sprite, a.facing, a.walking, a.animTick, flip);
    if (frame)
      pix::drawSprite(fb, *a.sprite, *frame, fx, fy, flip);
  }

} // namespace

void drawLevel(pix::Framebuffer &fb, const game::Level &level, const Player &p, std::vector<Actor> actors)
{
  drawGradient(fb, level.skyTop, level.skyBottom);

  // Camera: keep the player in the middle of the screen.
  // x moves in whole pixels and y in half pixels, so rounding y with a
  // quarter-pixel bias keeps it far from the rounding edge. That makes
  // every frame round the same way and the motion stays even.
  float sx = (p.x - p.y) * pix::TILE_W / 2;
  float sy = (p.x + p.y) * pix::TILE_H / 2;
  int camX = static_cast<int>(std::floor(sx + 0.5f));
  int camY = static_cast<int>(std::floor(sy + 0.25f));
  int offX = GAME_W / 2 - pix::TILE_W / 2 - camX;
  int offY = GAME_H / 2 + 16 - camY;

  const int W = level.width();
  const int H = level.height();

  // 1) All tiles, one diagonal at a time, back to front
  for (int s = 0; s <= W + H - 2; ++s)
  {
    for (int tx = std::max(0, s - (H - 1)); tx <= std::min(s, W - 1); ++tx)
    {
      int ty = s - tx;
      char c = level.at(tx, ty);
      if (c == game::TILE_VOID)
        continue;

      const game::TileType &t = game::tileType(c);
      pix::Point pt = pix::isoToScreen(tx, ty);
      pix::drawIsoBlock(fb, pt.x + offX, pt.y + offY, t.height,
                        (tx + ty) % 2 ? t.topAlt : t.top, t.left, t.right);
    }
  }

  // 2) Actors, back to front. After each one, redraw the tall tiles
  //    in front of it so it can walk behind them.
  std::sort(actors.begin(), actors.end(),
            [](const Actor &a, const Actor &b)
            { return a.x + a.y < b.x + b.y; });

  for (const Actor &a : actors)
  {
    int groundH = game::tileType(level.at(static_cast<int>(a.x), static_cast<int>(a.y))).height;
    float ax = (a.x - a.y) * pix::TILE_W / 2;
    float ay = (a.x + a.y) * pix::TILE_H / 2;
    int feetX = static_cast<int>(std::floor(ax + 0.5f)) + offX + pix::TILE_W / 2;
    int feetY = static_cast<int>(std::floor(ay + 0.25f)) + offY - groundH;
    drawActor(fb, a, feetX, feetY);

    float depth = a.x + a.y;
    for (int s = 0; s <= W + H - 2; ++s)
    {
      if (s + 1 <= depth)
        continue;

      for (int tx = std::max(0, s - (H - 1)); tx <= std::min(s, W - 1); ++tx)
      {
        int ty = s - tx;
        char c = level.at(tx, ty);
        if (c == game::TILE_VOID)
          continue;

        const game::TileType &t = game::tileType(c);
        if (t.height <= groundH)
          continue;

        pix::Point pt = pix::isoToScreen(tx, ty);
        pix::drawIsoBlock(fb, pt.x + offX, pt.y + offY - groundH, t.height - groundH,
                          (tx + ty) % 2 ? t.topAlt : t.top, t.left, t.right);
      }
    }
  }
}

void drawGame(pix::Framebuffer &fb, const Game &g)
{
  const game::Level &level = g.world.levels[g.player.floor];
  const Player &p = g.player;

  std::vector<Actor> actors;
  actors.push_back({&g.sprites.at("traveler"), p.x, p.y, p.facing, p.walking, p.animTick});

  for (const NpcState &npc : g.npcs)
  {
    auto it = g.sprites.find(npc.spawn->sprite);
    const pix::Sprite *sprite = (it != g.sprites.end()) ? &it->second : nullptr;
    actors.push_back({sprite, npc.x, npc.y, npc.facing, false, 0});
  }

  drawLevel(fb, level, p, actors);

  bool calm = (g.transition.phase == FadePhase::None);

  if (calm && !g.dialogue.active && !g.scene.active())
  {
    if (npcInReach(g) >= 0)
    {
      drawPrompt(fb, "talk");
    }
    else
    {
      ExitInfo ex = exitUnderPlayer(g);
      if (ex.valid)
      {
        if (ex.usable)
          drawPrompt(fb, ex.text);
        else
          drawNotice(fb, ex.text);
      }
    }
  }

  if (g.burden > 0)
    drawBurden(fb, g.burden);

  drawBanner(fb, g.banner);

  // Scene effects: the light, then any color over the screen
  drawGlow(fb, g.effects.glowRadius);
  if (g.effects.fadeAlpha > 0)
  {
    pix::Color cover = (g.effects.fadeColor & 0x00FFFFFFu) | (static_cast<pix::Color>(g.effects.fadeAlpha) << 24);
    fb.fillRect(0, 0, GAME_W, GAME_H, cover);
  }

  // Text goes on top of the effects, so it can be read over a white flash
  if (calm)
    drawDialogue(fb, g.dialogue);
  if (g.scene.choice)
    drawChoice(fb, *g.scene.choice, g.scene.choiceSelected);
  if (g.scene.captioning)
    drawCaption(fb, g.scene.caption, g.scene.tick);

  drawToast(fb, g.toast);

  // Stairs and death fades go over the world, the mission log over everything
  int fade = fadeAmount(g.transition);
  if (fade > 0)
  {
    pix::Color overlay = (g.transition.color & 0x00FFFFFFu) | (static_cast<pix::Color>(fade) << 24);
    fb.fillRect(0, 0, GAME_W, GAME_H, overlay);
  }

  if (g.logOpen)
    drawMissionLog(fb, g);
}
