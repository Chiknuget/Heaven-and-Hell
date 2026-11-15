#include "game/enemy.h"

#include <fstream>
#include <sstream>
#include <stdexcept>
#include <cctype>

namespace game
{

  namespace
  {

    std::string trim(const std::string &s)
    {
      size_t start = s.find_first_not_of(" \t\r");
      if (start == std::string::npos)
        return "";
      size_t end = s.find_last_not_of(" \t\r");
      return s.substr(start, end - start + 1);
    }

    [[noreturn]] void fail(const std::string &path, int lineNo, const std::string &msg)
    {
      throw std::runtime_error(path + ":" + std::to_string(lineNo) + ":" + msg);
    }

    template <typename T>
    T readNumber(const std::string &value, const std::string &path, int lineNo)
    {
      std::istringstream in(value);
      T number{};
      if (!(in >> number) || number < 0)
        fail(path, lineNo, "expected a positive number");
      return number;
    }

  } // namespace

  EnemyType loadEnemyType(const std::string &path)
  {
    std::ifstream in(path);
    if (!in)
      throw std::runtime_error("Could not open enemy file: " + path);

    EnemyType t;
    t.name = path;
    std::string raw;
    int lineNo = 0;

    while (std::getline(in, raw))
    {
      ++lineNo;
      std::string line = trim(raw);
      if (line.empty() || line.rfind("//", 0) == 0)
        continue;

      size_t colon = line.find(':');
      if (colon == std::string::npos)
        fail(path, lineNo, "expected 'key: value'");

      std::string key = trim(line.substr(0, colon));
      std::string value = trim(line.substr(colon + 1));

      // Allow a comment after the value
      size_t comment = value.find("//");
      if (comment != std::string::npos)
        value = trim(value.substr(0, comment));

      if (key == "name")
      {
        t.name = value;
      }
      else if (key == "sprite")
      {
        t.sprite = value;
      }
      else if (key == "health")
      {
        t.health = readNumber<int>(value, path, lineNo);
      }
      else if (key == "speed")
      {
        t.speed = readNumber<float>(value, path, lineNo);
      }
      else if (key == "damage")
      {
        t.damage = readNumber<int>(value, path, lineNo);
      }
      else if (key == "sight")
      {
        t.sight = readNumber<float>(value, path, lineNo);
      }
      else if (key == "radius")
      {
        t.radius = readNumber<float>(value, path, lineNo);
      }
      else if (key == "charge_speed")
      {
        t.chargeSpeed = readNumber<float>(value, path, lineNo);
      }
      else if (key == "behavior")
      {
        std::string lower;
        for (char ch : value)
          lower += static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));

        if (lower == "chase")
          t.behavior = EnemyBehavior::Chase;
        else if (lower == "charge")
          t.behavior = EnemyBehavior::Charge;
        else
          fail(path, lineNo, "behavior must be 'chase' or 'charge', not '" + value + "'");
      }
      else
      {
        fail(path, lineNo, "unknown key '" + key + "'");
      }
    }

    if (t.sprite.empty())
      throw std::runtime_error(path + ": enemy needs a 'sprite'");
    if (t.health <= 0)
      throw std::runtime_error(path + ": enemy health must be at least 1");
    return t;
  }

} // namespace game
