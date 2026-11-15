#pragma once
#include <string>

namespace game
{
  enum class EnemyBehavior
  {
    Chase,  // walks straight at the player once it notices them
    Charge, // winds up, then dashes in a line; dazed if it hits a wall
  };

  // One kind of enemy, loaded from enemies/<type>.txt
  struct EnemyType
  {
    std::string name;
    std::string sprite;
    int health = 3;
    float speed = 1.5f;  // tiles per second
    int damage = 1;      // hearts taken per hit
    float sight = 5.0f;  // notices the player within this many tiles
    float radius = 0.3f; // half the width of its body, in tiles
    EnemyBehavior behavior = EnemyBehavior::Chase;
    float chargeSpeed = 6.0f; // tiles per second while charging
  };

  // Throws std::runtime_error with the file name and line number on mistakes
  EnemyType loadEnemyType(const std::string &path);

} // namespace game
