#include "play/save.h"

#include "play/missions.h"
#include "play/talk.h"
#include "play/world.h"

#include <filesystem>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <stdexcept>

namespace
{

  const std::string SAVE_DIR = "saves";
  const std::string SAVE_PATH = "saves/save.txt";
  const std::string SAVE_HEADER = "heaven-and-hell-save";
  constexpr int SAVE_VERSION = 1; // bump this if the format ever changes

  [[noreturn]] void fail(int lineNo, const std::string &msg)
  {
    throw std::runtime_error(SAVE_PATH + ":" + std::to_string(lineNo) + ": " + msg);
  }

  const char *facingWord(Facing f)
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

  Facing facingFromWord(const std::string &word)
  {
    if (word == "up")
      return Facing::Up;
    if (word == "left")
      return Facing::Left;
    if (word == "right")
      return Facing::Right;
    return Facing::Down;
  }

  // Everything left on the line, for values that can contain spaces
  std::string restOfLine(std::istringstream &in)
  {
    std::string rest;
    std::getline(in, rest);
    size_t start = rest.find_first_not_of(' ');
    return start == std::string::npos ? "" : rest.substr(start);
  }

} // namespace

bool saveExists()
{
  return std::filesystem::exists(SAVE_PATH);
}

bool canSave(const Game &g)
{
  return g.inProgress && !g.scene.active() && !g.dialogue.active &&
         g.transition.phase == FadePhase::None;
}

void saveGame(const Game &g)
{
  std::filesystem::create_directories(SAVE_DIR);
  const std::string tempPath = SAVE_PATH + ".tmp";

  {
    std::ofstream out(tempPath);
    if (!out)
      throw std::runtime_error("Could not write " + tempPath);

    out << std::setprecision(9); // enough digits that positions come back exactly
    out << SAVE_HEADER << ' ' << SAVE_VERSION << '\n';
    out << "levels " << g.world.levels.size() << '\n';
    out << "name " << g.playerName << '\n';

    const Player &p = g.player;
    out << "player " << p.floor << ' ' << p.x << ' ' << p.y << ' ' << facingWord(p.facing) << '\n';
    out << "checkpoint " << g.checkpoint.floor << ' ' << g.checkpoint.x << ' ' << g.checkpoint.y << '\n';
    out << "burden " << g.burden << '\n';

    for (const auto &[flag, value] : g.flags)
      out << "flag " << flag << ' ' << value << '\n';
    for (const auto &[floor, count] : g.visits)
      out << "visit " << floor << ' ' << count << '\n';
    for (const auto &[name, count] : g.talks)
      out << "talk " << name << ' ' << count << '\n';

    for (const Mission &m : g.missions)
    {
      out << "mission " << m.id << ' ' << (m.done ? 1 : 0) << ' ' << m.title << '\n';
      if (!m.objective.empty())
        out << "objective " << m.id << ' ' << m.objective << '\n';
    }

    for (const TileChange &t : g.tileChanges)
      out << "tile " << t.floor << ' ' << t.x << ' ' << t.y << ' ' << t.tile << '\n';

    if (!out)
      throw std::runtime_error("Could not finish writing " + tempPath);
  }

  // Only replace the old save once the new one is completely written
  std::filesystem::rename(tempPath, SAVE_PATH);
}

void loadGame(Game &g)
{
  std::ifstream in(SAVE_PATH);
  if (!in)
    throw std::runtime_error("No saved game found");

  // Build the loaded game on the side, so a broken save can't ruin the current one
  Game loaded;
  loadAssets(loaded);
  const int levelCount = static_cast<int>(loaded.world.levels.size());

  auto checkFloor = [&](int floor, int lineNo)
  {
    if (floor < 0 || floor >= levelCount)
      fail(lineNo, "floor " + std::to_string(floor) + " doesn't exist");
  };

  std::string line;
  int lineNo = 0;
  bool hasPlayer = false;

  while (std::getline(in, line))
  {
    ++lineNo;
    if (line.empty())
      continue;

    std::istringstream s(line);
    std::string key;
    s >> key;

    if (lineNo == 1)
    {
      int version = 0;
      if (key != SAVE_HEADER || !(s >> version))
        fail(lineNo, "this isn't a save file");
      if (version != SAVE_VERSION)
        fail(lineNo, "this save is from a different version of the game");
      continue;
    }

    if (key == "levels")
    {
      int n = 0;
      if (!(s >> n) || n != levelCount)
        fail(lineNo, "the save was made with a different set of levels");
    }
    else if (key == "name")
    {
      loaded.playerName = restOfLine(s);
    }
    else if (key == "player")
    {
      std::string facing;
      if (!(s >> loaded.player.floor >> loaded.player.x >> loaded.player.y >> facing))
        fail(lineNo, "expected 'player floor x y facing'");
      checkFloor(loaded.player.floor, lineNo);
      loaded.player.facing = facingFromWord(facing);
      hasPlayer = true;
    }
    else if (key == "checkpoint")
    {
      Destination &c = loaded.checkpoint;
      if (!(s >> c.floor >> c.x >> c.y))
        fail(lineNo, "expected 'checkpoint floor x y'");
      checkFloor(c.floor, lineNo);
    }
    else if (key == "burden")
    {
      if (!(s >> loaded.burden))
        fail(lineNo, "expected 'burden number'");
    }
    else if (key == "flag")
    {
      std::string flag;
      int value = 0;
      if (!(s >> flag >> value))
        fail(lineNo, "expected 'flag name number'");
      loaded.flags[flag] = value;
    }
    else if (key == "visit")
    {
      int floor = 0, count = 0;
      if (!(s >> floor >> count))
        fail(lineNo, "expected 'visit floor count'");
      checkFloor(floor, lineNo);
      loaded.visits[floor] = count;
    }
    else if (key == "talk")
    {
      std::string name;
      int count = 0;
      if (!(s >> name >> count))
        fail(lineNo, "expected 'talk name count'");
      loaded.talks[name] = count;
    }
    else if (key == "mission")
    {
      Mission m;
      int done = 0;
      if (!(s >> m.id >> done))
        fail(lineNo, "expected 'mission id done title'");
      m.done = (done != 0);
      m.title = restOfLine(s);
      loaded.missions.push_back(m);
    }
    else if (key == "objective")
    {
      std::string id;
      if (!(s >> id))
        fail(lineNo, "expected 'objective id text'");
      setObjective(loaded, id, restOfLine(s));
    }
    else if (key == "tile")
    {
      int floor = 0, x = 0, y = 0;
      std::string tile;
      if (!(s >> floor >> x >> y >> tile) || tile.size() != 1)
        fail(lineNo, "expected 'tile floor x y tile'");
      checkFloor(floor, lineNo);
      setTile(loaded, floor, x, y, tile[0]);
    }
    else
    {
      fail(lineNo, "unknown entry '" + key + "'");
    }
  }

  if (lineNo == 0 || !hasPlayer || loaded.playerName.empty())
    throw std::runtime_error(SAVE_PATH + ": the save is incomplete");

  // Put the player back where they were, without replaying anything that
  // happens on arrival
  refreshNpcs(loaded);
  const game::Level &level = loaded.world.levels[loaded.player.floor];
  showBanner(loaded.banner, level.name, level.subtitle);
  loaded.inProgress = true;

  g = std::move(loaded);
}
