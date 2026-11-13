#include "game/level.h"

#include <algorithm>
#include <fstream>
#include <sstream>
#include <stdexcept>

namespace game
{

	using pix::rgb;

	// Each entry: top, top (alternate), left side, right side, height, walkable, [deadly]
	const TileType &tileType(char c)
	{
		switch (c)
		{
		// Heaven
		case '.':
		{
			static const TileType t{rgb(238, 236, 228), rgb(226, 224, 216), rgb(180, 176, 190), rgb(205, 200, 210), 4, true};
			return t;
		} // cloud
		case 'g':
		{
			static const TileType t{rgb(236, 200, 90), rgb(226, 188, 78), rgb(160, 120, 40), rgb(196, 150, 56), 4, true};
			return t;
		} // gold path
		case 'P':
		{
			static const TileType t{rgb(250, 248, 240), rgb(250, 248, 240), rgb(190, 186, 200), rgb(220, 216, 228), 22, false};
			return t;
		} // marble pillar

		// Surface
		case ',':
		{
			static const TileType t{rgb(96, 168, 72), rgb(88, 156, 66), rgb(70, 52, 36), rgb(92, 70, 46), 4, true};
			return t;
		} // grass
		case 'd':
		{
			static const TileType t{rgb(170, 130, 84), rgb(160, 122, 78), rgb(100, 74, 46), rgb(126, 94, 60), 4, true};
			return t;
		} // dirt path
		case 'T':
		{
			static const TileType t{rgb(40, 110, 50), rgb(40, 110, 50), rgb(24, 70, 32), rgb(32, 90, 40), 18, false};
			return t;
		} // tree
		case 'w':
		{
			static const TileType t{rgb(64, 120, 200), rgb(58, 110, 190), rgb(30, 60, 110), rgb(40, 80, 140), 2, false};
			return t;
		} // water

		// Caves
		case ':':
		{
			static const TileType t{rgb(110, 100, 96), rgb(102, 92, 88), rgb(60, 54, 52), rgb(80, 72, 68), 4, true};
			return t;
		} // cave floor
		case '#':
		{
			static const TileType t{rgb(86, 78, 74), rgb(86, 78, 74), rgb(44, 40, 38), rgb(62, 56, 52), 20, false};
			return t;
		} // cave wall

		// Deep Caves
		case ';':
		{
			static const TileType t{rgb(64, 62, 78), rgb(58, 56, 72), rgb(30, 28, 40), rgb(44, 42, 56), 4, true};
			return t;
		} // dark stone
		case '%':
		{
			static const TileType t{rgb(48, 46, 60), rgb(48, 46, 60), rgb(20, 18, 28), rgb(32, 30, 42), 20, false};
			return t;
		} // basalt wall
		case '*':
		{
			static const TileType t{rgb(120, 230, 240), rgb(120, 230, 240), rgb(40, 120, 150), rgb(70, 170, 200), 16, false};
			return t;
		} // crystal

		// Hell (shared by all circles)
		case 'r':
		{
			static const TileType t{rgb(92, 34, 30), rgb(84, 30, 28), rgb(40, 14, 14), rgb(60, 22, 20), 4, true};
			return t;
		} // hellrock
		case '~':
		{
			static const TileType t{rgb(255, 120, 20), rgb(240, 96, 16), rgb(150, 40, 10), rgb(190, 60, 12), 2, true, true};
			return t;
		} // lava (deadly)
		case 'O':
		{
			static const TileType t{rgb(54, 40, 60), rgb(54, 40, 60), rgb(20, 12, 24), rgb(34, 24, 40), 20, false};
			return t;
		} // obsidian

		// The nine circles
		case 'a':
		{
			static const TileType t{rgb(130, 126, 122), rgb(120, 116, 112), rgb(70, 68, 66), rgb(95, 92, 88), 4, true};
			return t;
		} // ash (Limbo)
		case 'k':
		{
			static const TileType t{rgb(88, 60, 110), rgb(80, 54, 100), rgb(40, 26, 54), rgb(60, 40, 78), 4, true};
			return t;
		} // storm stone (Lust)
		case 'm':
		{
			static const TileType t{rgb(92, 70, 44), rgb(84, 64, 40), rgb(46, 34, 20), rgb(64, 48, 30), 4, true};
			return t;
		} // mud (Gluttony, Wrath)
		case 'C':
		{
			static const TileType t{rgb(230, 190, 60), rgb(230, 190, 60), rgb(150, 110, 30), rgb(190, 150, 45), 10, false};
			return t;
		} // coin heap (Greed)
		case 'x':
		{
			static const TileType t{rgb(50, 70, 40), rgb(44, 62, 36), rgb(22, 32, 18), rgb(34, 48, 28), 2, true, true};
			return t;
		} // Styx swamp (deadly)
		case 'B':
		{
			static const TileType t{rgb(120, 60, 40), rgb(120, 60, 40), rgb(60, 24, 16), rgb(90, 36, 24), 12, false};
			return t;
		} // burning tomb (Heresy)
		case 'b':
		{
			static const TileType t{rgb(150, 10, 20), rgb(135, 8, 18), rgb(70, 4, 8), rgb(100, 6, 12), 2, true, true};
			return t;
		} // river of blood (deadly)
		case 'f':
		{
			static const TileType t{rgb(70, 50, 50), rgb(64, 46, 46), rgb(32, 22, 22), rgb(48, 34, 34), 4, true};
			return t;
		} // ditch stone (Fraud)
		case 'i':
		{
			static const TileType t{rgb(190, 220, 240), rgb(176, 208, 232), rgb(100, 140, 180), rgb(140, 180, 210), 4, true};
			return t;
		} // ice (Treachery)

		// Purgatory
		case 'p':
		{
			static const TileType t{rgb(200, 196, 184), rgb(190, 186, 174), rgb(120, 116, 108), rgb(156, 150, 140), 4, true};
			return t;
		} // pale stone
		case 'G':
		{
			static const TileType t{rgb(255, 250, 210), rgb(255, 250, 210), rgb(200, 180, 110), rgb(230, 210, 140), 6, true};
			return t;
		} // gate
		case '$':
		{
			static const TileType t{rgb(170, 40, 50), rgb(170, 40, 50), rgb(90, 16, 24), rgb(130, 26, 36), 6, true};
			return t;
		} // toll gate
		case 'F':
		{
			static const TileType t{rgb(255, 170, 60), rgb(250, 150, 40), rgb(200, 90, 20), rgb(230, 120, 30), 3, true};
			return t;
		} // purifying fire (safe)

		// Stairs (work on every stacked floor)
		case '^':
		{
			static const TileType t{rgb(230, 230, 240), rgb(230, 230, 240), rgb(150, 150, 170), rgb(190, 190, 205), 6, true};
			return t;
		} // stairs up
		case 'v':
		{
			static const TileType t{rgb(24, 20, 28), rgb(24, 20, 28), rgb(12, 10, 14), rgb(18, 16, 22), 1, true};
			return t;
		} // stairs down

		case ' ':
		{
			static const TileType t{0, 0, 0, 0, 0, false};
			return t;
		} // void
		default:
		{
			static const TileType t{rgb(255, 0, 255), rgb(255, 0, 255), rgb(128, 0, 128), rgb(192, 0, 192), 4, false};
			return t;
		} // typo = magenta
		}
	}

	int Level::width() const
	{
		size_t w = 0;
		for (const std::string &row : rows)
			w = std::max(w, row.size());
		return static_cast<int>(w);
	}

	char Level::at(int tx, int ty) const
	{
		if (ty < 0 || ty >= height())
			return TILE_VOID;
		const std::string &row = rows[ty];
		if (tx < 0 || tx >= static_cast<int>(row.size()))
			return TILE_VOID;
		return row[tx];
	}

	bool Level::isWalkable(int tx, int ty) const
	{
		return tileType(at(tx, ty)).walkable;
	}

	int Level::connectorIndex(int tx, int ty) const
	{
		char c = at(tx, ty);
		int n = 0;
		for (int y = 0; y < height(); ++y)
		{
			for (int x = 0; x < static_cast<int>(rows[y].size()); ++x)
			{
				if (rows[y][x] != c)
					continue;
				if (x == tx && y == ty)
					return n;
				++n;
			}
		}
		return -1;
	}

	bool Level::findNth(char c, int n, int &outX, int &outY) const
	{
		for (int y = 0; y < height(); ++y)
		{
			for (int x = 0; x < static_cast<int>(rows[y].size()); ++x)
			{
				if (rows[y][x] != c)
					continue;
				if (n == 0)
				{
					outX = x;
					outY = y;
					return true;
				}
				--n;
			}
		}
		return false;
	}

	namespace
	{

		std::string trim(const std::string &s)
		{
			size_t start = s.find_first_not_of(" \t");
			if (start == std::string::npos)
				return "";
			size_t end = s.find_last_not_of(" \t");
			return s.substr(start, end - start + 1);
		}

		[[noreturn]] void fail(const std::string &path, int lineNo, const std::string &msg)
		{
			throw std::runtime_error(path + ":" + std::to_string(lineNo) + ": " + msg);
		}

		pix::Color parseColor(const std::string &text, const std::string &path, int lineNo)
		{
			std::istringstream in(text);
			int r, g, b;
			if (!(in >> r >> g >> b) || r < 0 || r > 255 || g < 0 || g > 255 || b < 0 || b > 255)
				fail(path, lineNo, "expected a color like '120 180 240'");
			return pix::rgb(r, g, b);
		}

		// Removes the '\r' that Windows line endings leave behind
		void stripCR(std::string &line)
		{
			if (!line.empty() && line.back() == '\r')
				line.pop_back();
		}

	} // namespace

	Level loadLevel(const std::string &path)
	{
		std::ifstream in(path);
		if (!in)
			throw std::runtime_error("Could not open level file: " + path);

		Level level;
		level.name = path;

		bool inMap = false;
		bool hasSpawn = false;
		std::string line;
		int lineNo = 0;

		while (std::getline(in, line))
		{
			++lineNo;
			stripCR(line);

			// Everything after "map:" is the map, exactly as written
			if (inMap)
			{
				level.rows.push_back(line);
				continue;
			}

			std::string trimmed = trim(line);
			if (trimmed.empty() || trimmed.rfind("//", 0) == 0)
				continue; // blank or comment

			size_t colon = trimmed.find(':');
			if (colon == std::string::npos)
				fail(path, lineNo, "expected 'key: value'");

			std::string key = trim(trimmed.substr(0, colon));
			std::string value = trim(trimmed.substr(colon + 1));

			if (key == "name")
			{
				level.name = value;
			}
			else if (key == "subtitle")
			{
				level.subtitle = value;
			}
			else if (key == "sky_top")
			{
				level.skyTop = parseColor(value, path, lineNo);
			}
			else if (key == "sky_bottom")
			{
				level.skyBottom = parseColor(value, path, lineNo);
			}
			else if (key == "spawn")
			{
				std::istringstream s(value);
				if (!(s >> level.spawnX >> level.spawnY))
					fail(path, lineNo, "expected a tile position like '4 4'");
				hasSpawn = true;
			}
			else if (key == "npc")
			{
				std::istringstream s(value);
				NpcSpawn npc;
				if (!(s >> npc.x >> npc.y >> npc.name))
					fail(path, lineNo, "expected 'npc: x y name'");

				std::string option;
				while (s >> option)
				{
					if (option == "first_visit")
						npc.firstVisitOnly = true;
					else if (option == "greet")
						npc.greet = true;
					else
						fail(path, lineNo, "unknown npc option '" + option + "'");
				}

				npc.line = lineNo;
				level.npcs.push_back(npc);
			}
			else if (key == "map")
			{
				inMap = true;
			}
			else
			{
				fail(path, lineNo, "unknown key '" + key + "'");
			}
		}

		// Ignore blank lines at the end of the file
		while (!level.rows.empty() && trim(level.rows.back()).empty())
			level.rows.pop_back();

		if (level.rows.empty())
			throw std::runtime_error(path + ": level has no map");

		if (hasSpawn)
		{
			if (!level.isWalkable(level.spawnX, level.spawnY))
				throw std::runtime_error(path + ": spawn point is not on a walkable tile");
		}
		else
		{
			// No spawn given: use the first walkable, safe tile
			level.spawnX = -1;
			for (int y = 0; y < level.height() && level.spawnX < 0; ++y)
			{
				for (int x = 0; x < level.width(); ++x)
				{
					if (level.isWalkable(x, y) && !tileType(level.at(x, y)).deadly)
					{
						level.spawnX = x;
						level.spawnY = y;
						break;
					}
				}
			}
			if (level.spawnX < 0)
				throw std::runtime_error(path + ": level has no walkable tiles");
		}

		// Characters have to stand on solid ground
		for (const NpcSpawn &npc : level.npcs)
		{
			if (!level.isWalkable(npc.x, npc.y))
				fail(path, npc.line, "npc '" + npc.name + "' is not on a walkable tile");
		}

		return level;
	}

	World loadWorld(const std::string &worldPath)
	{
		std::ifstream in(worldPath);
		if (!in)
			throw std::runtime_error("Could not open world file: " + worldPath);

		// Level files are found next to the world file
		std::string dir;
		size_t slash = worldPath.find_last_of('/');
		if (slash != std::string::npos)
			dir = worldPath.substr(0, slash + 1);

		World world;
		std::vector<std::string> purgatoryFiles;
		std::string line;

		while (std::getline(in, line))
		{
			stripCR(line);
			std::string name = trim(line);
			if (name.empty() || name.rfind("//", 0) == 0)
				continue;

			// "purgatory: file.txt" lines build Purgatory's own stack
			if (name.rfind("purgatory:", 0) == 0)
			{
				purgatoryFiles.push_back(trim(name.substr(10)));
				continue;
			}

			world.levels.push_back(loadLevel(dir + name));
		}

		world.stackSize = static_cast<int>(world.levels.size());
		if (world.stackSize == 0)
			throw std::runtime_error(worldPath + ": no levels listed");

		if (!purgatoryFiles.empty())
		{
			world.purgatoryTop = static_cast<int>(world.levels.size());
			for (const std::string &file : purgatoryFiles)
			{
				world.levels.push_back(loadLevel(dir + file));
			}
			world.purgatoryBottom = static_cast<int>(world.levels.size()) - 1;
		}

		return world;
	}

} // namespace game
