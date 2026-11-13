#pragma once
#include "gfx/framebuffer.h"
#include <string>
#include <vector>

namespace game
{

	// Main floors from the top, as indexes into World::levels.
	// The order must match levels/world.txt.
	enum Floor
	{
		HEAVEN = 0,
		SURFACE,
		CAVES,
		DEEP_CAVES,
		FIRST_CIRCLE // the nine circles of hell follow in order
	};

	constexpr char TILE_VOID = ' ';
	constexpr char TILE_UP = '^';
	constexpr char TILE_DOWN = 'v';
	constexpr char TILE_GATE = 'G'; // free return to your checkpoint (Purgatory's summit)
	constexpr char TILE_TOLL = '$'; // instant return to your checkpoint, for a price

	struct TileType
	{
		pix::Color top;
		pix::Color topAlt; // checkerboard shade
		pix::Color left;
		pix::Color right;
		int height;
		bool walkable;
		bool deadly = false; // walking into it sends you to Purgatory
	};

	const TileType &tileType(char c);

	// A character placed in a level file with "npc: x y name [options]"
	struct NpcSpawn
	{
		int x = 0;
		int y = 0;
		std::string name;						 // their scene or dialogue file
		std::string sprite;					 // their sprite file (defaults to name)
		std::string ifFlag;					 // only present when this flag is set
		std::string unlessFlag;			 // only present when this flag is not set
		bool firstVisitOnly = false; // only there the first time you enter the level
		bool greet = false;					 // starts talking as soon as you arrive
		int line = 0;								 // line in the level file, for error messages
	};

	struct Level
	{
		std::string name;
		std::string subtitle; // optional second line on the floor banner
		pix::Color skyTop = pix::rgb(0, 0, 0);
		pix::Color skyBottom = pix::rgb(0, 0, 0);
		int spawnX = -1; // where you appear when you don't arrive by stairs
		int spawnY = -1;
		std::string enterScene; // scene to run on arrival, if any
		bool enterOnce = false; // only on the first visit
		std::vector<NpcSpawn> npcs;
		std::vector<std::string> rows;

		int width() const;
		int height() const { return static_cast<int>(rows.size()); }

		char at(int tx, int ty) const; // TILE_VOID if outside the map
		bool isWalkable(int tx, int ty) const;

		// For stairs: which number this tile is among tiles of the same kind,
		// counting left to right, top to bottom (0 = first)
		int connectorIndex(int tx, int ty) const;

		// Finds the nth tile of kind c. Returns false if there aren't that many.
		bool findNth(char c, int n, int &outX, int &outY) const;
	};

	struct World
	{
		std::vector<Level> levels; // main floors first, then Purgatory's levels
		int stackSize = 0;				 // how many main floors there are
		int purgatoryTop = -1;		 // Purgatory's summit, or -1 if there's no Purgatory
		int purgatoryBottom = -1;	 // where the dead arrive

		bool inPurgatory(int floor) const { return purgatoryTop >= 0 && floor >= purgatoryTop; }

		// Stairs only connect levels in the same stack
		bool sameStack(int a, int b) const { return inPurgatory(a) == inPurgatory(b); }
	};

	// Loads one level from a text file. Throws std::runtime_error with the
	// file name and line number if something is wrong.
	Level loadLevel(const std::string &path);

	// Loads the world described by a world file. Level paths are relative
	// to the world file's folder.
	World loadWorld(const std::string &worldPath);

} // namespace game
