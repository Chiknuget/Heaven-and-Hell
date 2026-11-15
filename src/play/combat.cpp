#include "play/combat.h"

#include "play/movement.h"
#include "play/talk.h"
#include "play/text.h"
#include "play/scenes.h"

#include <cmath>
#include <vector>

namespace
{

  constexpr int WINDUP_FRAMES = 40;  // how long a charger warns before it charges
  constexpr int CHARGE_FRAMES = 45;  // longest a charge lasts
  constexpr int RECOVER_FRAMES = 40; // rest after a charge that missed
  constexpr int DAZED_FRAMES = 100;  // rest after slamming into a wall

  const std::vector<NpcState> NO_NPCS; // enemies walk through characters

  float distance(float ax, float ay, float bx, float by)
  {
    float dx = ax - bx;
    float dy = ay - by;
    return std::sqrt(dx * dx + dy * dy);
  }

  // The direction a facing points in the world, one tile long
  void facingVector(Facing f, float &dx, float &dy)
  {
    const float k = 0.70710678f; // 1 / sqrt(2)
    switch (f)
    {
    case Facing::Right:
      dx = k;
      dy = -k;
      break;
    case Facing::Left:
      dx = -k;
      dy = k;
      break;
    case Facing::Up:
      dx = -k;
      dy = -k;
      break;
    default:
      dx = k;
      dy = k;
      break;
    }
  }

  // Moves an enemy, keeping it out of walls and off deadly ground.
  // Returns false if it couldn't go the whole way because something was in the way.
  bool moveEnemy(const game::Level &level, Enemy &e, float dx, float dy)
  {
    float oldX = e.x;
    float oldY = e.y;
    moveWithCollision(level, NO_NPCS, e.x, e.y, dx, dy, e.type->radius);

    char under = level.at(static_cast<int>(std::floor(e.x)), static_cast<int>(std::floor(e.y)));
    if (game::tileType(under).deadly)
    {
      e.x = oldX;
      e.y = oldY;
      return false;
    }

    float wanted = std::sqrt(dx * dx + dy * dy);
    return distance(oldX, oldY, e.x, e.y) >= wanted * 0.9f;
  }

  void hurtPlayer(Game &g, int damage, float fromX, float fromY)
  {
    Player &p = g.player;
    if (p.hurtTimer > 0 || damage <= 0)
      return;

    p.health -= damage;
    p.hurtTimer = HURT_FRAMES;

    // Knocked away from whatever hit them
    float dx = p.x - fromX;
    float dy = p.y - fromY;
    float len = std::sqrt(dx * dx + dy * dy);
    if (len < 0.001f)
    {
      dx = 0.0f;
      dy = 1.0f;
      len = 1.0f;
    }
    p.knockX = dx / len * KNOCKBACK;
    p.knockY = dy / len * KNOCKBACK;
    p.knockTimer = KNOCKBACK_FRAMES;
  }

  void startWindup(Enemy &e)
  {
    e.mode = EnemyMode::Windup;
    e.timer = WINDUP_FRAMES;
  }

  // The player's swing lands in front of them and hurts every enemy it touches
  void playerAttack(Game &g)
  {
    Player &p = g.player;
    float fx = 0.0f, fy = 0.0f;
    facingVector(p.facing, fx, fy);
    float hitX = p.x + fx * ATTACK_REACH;
    float hitY = p.y + fy * ATTACK_REACH;

    for (Enemy &e : g.enemies)
    {
      if (e.hurtTimer > 0)
        continue;
      if (distance(hitX, hitY, e.x, e.y) > ATTACK_RADIUS + e.type->radius)
        continue;

      e.health -= 1;
      e.hurtTimer = ENEMY_HURT_FRAMES;

      float dx = e.x - p.x;
      float dy = e.y - p.y;
      float len = std::sqrt(dx * dx + dy * dy);
      if (len > 0.001f)
      {
        e.knockX = dx / len * KNOCKBACK;
        e.knockY = dy / len * KNOCKBACK;
        e.knockTimer = KNOCKBACK_FRAMES;
      }

      // A hit stops a charge in its tracks
      if (e.mode == EnemyMode::Charge || e.mode == EnemyMode::Windup)
      {
        e.mode = EnemyMode::Recover;
        e.timer = RECOVER_FRAMES;
      }
    }

    // Remove the defeated, setting their flags and starting their scenes
    for (size_t i = 0; i < g.enemies.size();)
    {
      if (g.enemies[i].health > 0)
      {
        ++i;
        continue;
      }

      const game::EnemySpawn *spawn = g.enemies[i].spawn;
      g.enemies.erase(g.enemies.begin() + i);

      if (!spawn->deathFlag.empty())
      {
        g.flags[spawn->deathFlag] = 1;
        refreshNpcs(g);
      }
      if (!spawn->deathScene.empty())
        startScene(g, spawn->deathScene);
    }
  }

  void updateEnemy(Game &g, Enemy &e)
  {
    const game::Level &level = g.world.levels[g.player.floor];
    const game::EnemyType &t = *e.type;
    const Player &p = g.player;

    if (e.hurtTimer > 0)
      --e.hurtTimer;
    e.walking = false;

    // Being knocked back takes over everything else
    if (e.knockTimer > 0)
    {
      --e.knockTimer;
      moveEnemy(level, e, e.knockX, e.knockY);
      return;
    }

    float dx = p.x - e.x;
    float dy = p.y - e.y;
    float dist = std::sqrt(dx * dx + dy * dy);
    bool sees = dist <= t.sight;

    switch (e.mode)
    {
    case EnemyMode::Idle:
      if (sees)
      {
        if (t.behavior == game::EnemyBehavior::Charge)
          startWindup(e);
        else
          e.mode = EnemyMode::Chase;
      }
      break;

    case EnemyMode::Chase:
      if (dist > t.sight * 1.5f)
      {
        e.mode = EnemyMode::Idle; // lost track of the player
        break;
      }
      if (dist > 0.01f)
      {
        float step = t.speed / 60.0f;
        moveEnemy(level, e, dx / dist * step, dy / dist * step);
        e.facing = facingToward(e.x, e.y, p.x, p.y);
        e.walking = true;
        ++e.animTick;
      }
      break;

    case EnemyMode::Windup:
      e.facing = facingToward(e.x, e.y, p.x, p.y);
      if (--e.timer <= 0)
      {
        // Lock in the direction: it can be dodged
        if (dist > 0.01f)
        {
          e.dirX = dx / dist;
          e.dirY = dy / dist;
        }
        e.mode = EnemyMode::Charge;
        e.timer = CHARGE_FRAMES;
      }
      break;

    case EnemyMode::Charge:
    {
      float step = t.chargeSpeed / 60.0f;
      bool clear = moveEnemy(level, e, e.dirX * step, e.dirY * step);
      e.walking = true;
      e.animTick += 2; // legs move faster while charging

      if (!clear)
      {
        e.mode = EnemyMode::Recover; // slammed into something: dazed
        e.timer = DAZED_FRAMES;
      }
      else if (--e.timer <= 0)
      {
        e.mode = EnemyMode::Recover;
        e.timer = RECOVER_FRAMES;
      }
      break;
    }

    case EnemyMode::Recover:
      if (--e.timer <= 0)
      {
        if (!sees)
          e.mode = EnemyMode::Idle;
        else if (t.behavior == game::EnemyBehavior::Charge)
          startWindup(e);
        else
          e.mode = EnemyMode::Chase;
      }
      break;
    }

    // Touching the player hurts them, except while recovering
    if (e.mode != EnemyMode::Recover && distance(e.x, e.y, p.x, p.y) < t.radius + PLAYER_RADIUS + 0.05f)
      hurtPlayer(g, t.damage, e.x, e.y);
  }

} // namespace

void spawnEnemies(Game &g)
{
  g.enemies.clear();
  const game::Level &level = g.world.levels[g.player.floor];

  for (const game::EnemySpawn &spawn : level.enemies)
  {
    if (!spawn.ifFlag.empty() && flagValue(g, spawn.ifFlag) == 0)
      continue;
    if (!spawn.unlessFlag.empty() && flagValue(g, spawn.unlessFlag) != 0)
      continue;

    auto it = g.enemyTypes.find(spawn.type);
    if (it == g.enemyTypes.end())
      continue;

    Enemy e;
    e.spawn = &spawn;
    e.type = &it->second;
    e.x = spawn.x + 0.5f;
    e.y = spawn.y + 0.5f;
    e.health = it->second.health;
    g.enemies.push_back(e);
  }
}

void updateCombat(const pix::Window &window, Game &g)
{
  Player &p = g.player;
  const game::Level &level = g.world.levels[p.floor];

  if (p.hurtTimer > 0)
    --p.hurtTimer;
  if (p.cooldown > 0)
    --p.cooldown;
  if (p.attackTimer > 0)
    --p.attackTimer;

  // Being knocked back
  if (p.knockTimer > 0)
  {
    --p.knockTimer;
    moveWithCollision(level, g.npcs, p.x, p.y, p.knockX, p.knockY);
  }

  // Space swings
  if (window.wasKeyPressed(pix::Key::Space) && p.cooldown == 0)
  {
    p.attackTimer = ATTACK_FRAMES;
    p.cooldown = ATTACK_COOLDOWN;
    playerAttack(g);
  }

  for (Enemy &e : g.enemies)
    updateEnemy(g, e);
}
