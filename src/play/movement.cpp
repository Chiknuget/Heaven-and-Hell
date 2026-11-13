#include "play/movement.h"

#include <cmath>

bool boxBlocked(const game::Level &level, float x, float y)
{
  int x0 = static_cast<int>(std::floor(x - PLAYER_RADIUS));
  int x1 = static_cast<int>(std::floor(x + PLAYER_RADIUS));
  int y0 = static_cast<int>(std::floor(y - PLAYER_RADIUS));
  int y1 = static_cast<int>(std::floor(y + PLAYER_RADIUS));

  for (int ty = y0; ty <= y1; ++ty)
    for (int tx = x0; tx <= x1; ++tx)
      if (!level.isWalkable(tx, ty))
        return true;

  return false;
}

bool npcBlocked(const std::vector<NpcState> &npcs, float x, float y)
{
  const float reach = PLAYER_RADIUS + NPC_RADIUS;
  for (const NpcState &npc : npcs)
  {
    if (std::fabs(x - npc.x) < reach && std::fabs(y - npc.y) < reach)
      return true;
  }
  return false;
}

void moveWithCollision(const game::Level &level, const std::vector<NpcState> &npcs,
                       float &px, float &py, float dx, float dy)
{
  // If the player somehow starts overlapping someone, let them walk free
  bool startedInside = npcBlocked(npcs, px, py);
  auto hitsNpc = [&](float x, float y)
  { return !startedInside && npcBlocked(npcs, x, y); };

  float nx = px + dx;
  if (boxBlocked(level, nx, py))
  {
    if (dx > 0)
      nx = std::floor(nx + PLAYER_RADIUS) - PLAYER_RADIUS - 0.001f;
    else if (dx < 0)
      nx = std::floor(nx - PLAYER_RADIUS) + 1.0f + PLAYER_RADIUS + 0.001f;
  }
  if (!hitsNpc(nx, py))
    px = nx;

  float ny = py + dy;
  if (boxBlocked(level, px, ny))
  {
    if (dy > 0)
      ny = std::floor(ny + PLAYER_RADIUS) - PLAYER_RADIUS - 0.001f;
    else if (dy < 0)
      ny = std::floor(ny - PLAYER_RADIUS) + 1.0f + PLAYER_RADIUS + 0.001f;
  }
  if (!hitsNpc(px, ny))
    py = ny;
}
