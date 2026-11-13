#pragma once
#include "game/dialogue.h"
#include "game/level.h"
#include "game/scene.h"
#include "gfx/framebuffer.h"
#include "gfx/sprite.h"

#include <map>
#include <string>
#include <vector>

// Everything the running game keeps track of, plus the numbers that tune it.

constexpr int GAME_W = 320;
constexpr int GAME_H = 180;
constexpr int START_SCALE = 4;

// Screen pixels moved per frame when walking sideways. Up/down moves half
// this, which matches the 2:1 slope of the iso grid. Keep it a whole number.
constexpr float SPEED_X = 1.0f;
constexpr float PLAYER_RADIUS = 0.2f; // half the width of the player's collision box, in tiles
constexpr float NPC_RADIUS = 0.3f;    // half the width of an NPC's collision box
constexpr float TALK_RANGE = 1.0f;    // how close you need to be to talk, in tiles
constexpr float EXPLORE_REACH = 0.8f; // how close counts as reaching a scene's explore target

constexpr int STAIRS_FADE = 20;    // frames for each half of a stairs fade
constexpr int DEATH_FADE = 45;     // slower fade when you die
constexpr int BANNER_FRAMES = 150; // how long the floor name stays up
constexpr int BANNER_FADE = 30;    // banner fade in/out time, inside BANNER_FRAMES
constexpr int TOAST_FRAMES = 200;  // how long a mission notice stays up
constexpr int TOAST_FADE = 30;

constexpr int DIALOGUE_CHARS = 48; // characters per line in the dialogue box
constexpr int DIALOGUE_LINES = 3;  // lines per page

constexpr int WALK_FRAME_TICKS = 8; // frames each walking pose is shown for
constexpr int MAX_NAME_LENGTH = 12; // longest name the player can choose

enum class Facing
{
  Down,
  Up,
  Left,
  Right
};

struct Player
{
  int floor;  // index into world.levels
  float x, y; // position on that floor, in tiles
  Facing facing = Facing::Down;
  bool walking = false;
  int animTick = 0; // counts up while walking
};

// A character standing on the current level
struct NpcState
{
  const game::NpcSpawn *spawn; // where they came from in the level file
  float x, y;
  Facing facing = Facing::Down;
};

// A place the player can be sent to
struct Destination
{
  int floor = 0;
  float x = 0.0f, y = 0.0f;
};

// Something under the player that E can activate (stairs or a gate)
struct ExitInfo
{
  bool valid = false;  // there's something here
  bool usable = false; // E works; if false, text explains why not
  std::string text;    // "ascend", or the reason it's blocked
  Destination dest;
  int burdenCost = 0; // Burden added for using it
};

enum class FadePhase
{
  None,
  FadeOut, // screen fading away, player frozen
  FadeIn   // at the destination, screen coming back
};

struct Transition
{
  FadePhase phase = FadePhase::None;
  int frame = 0;
  int length = STAIRS_FADE;             // frames per half
  pix::Color color = pix::rgb(0, 0, 0); // what the screen fades to
  Destination dest;
};

struct Banner
{
  std::string title;
  std::string subtitle;
  int frame = BANNER_FRAMES; // starts "finished", so nothing shows
};

// A short notice in the top-left corner, like "New mission"
struct Toast
{
  std::string text;
  int frame = TOAST_FRAMES; // starts "finished", so nothing shows
};

struct Mission
{
  std::string id;
  std::string title;
  std::string objective; // may contain {flags}, filled in when shown
  bool done = false;
};

struct DialogueState
{
  bool active = false;
  std::string speaker;
  std::vector<std::string> pages;
  int page = 0;
  int shown = 0; // letters revealed so far on this page
  int tick = 0;  // frame counter, for the blinking arrow
};

enum class Tween
{
  None,
  Fade,
  Glow
};

// Screen effects that scenes control
struct Effects
{
  pix::Color fadeColor = pix::rgb(0, 0, 0);
  int fadeAlpha = 0;  // 0 = no color over the screen, 255 = fully covered
  int glowRadius = 0; // 0 = no light

  // A change in progress (scenes wait for it to finish)
  Tween tween = Tween::None;
  int frame = 0, length = 1;
  pix::Color fromColor = 0, toColor = 0;
  int from = 0, to = 0;
};

struct SceneState
{
  const game::Scene *scene = nullptr; // nullptr = no scene running
  int pc = 0;                         // index of the next command
  int waitFrames = 0;
  bool exploring = false;
  float exploreX = 0.0f, exploreY = 0.0f;
  const game::SceneCommand *choice = nullptr; // the choice on screen, if any
  int choiceSelected = 0;
  bool captioning = false;
  std::string caption;
  int tick = 0;

  bool active() const { return scene != nullptr; }
};

// Anything standing in the world that needs depth sorting
struct Actor
{
  const pix::Sprite *sprite;
  float x, y;
  Facing facing;
  bool walking;
  int animTick;
};

// A tile a scene changed, remembered so saves can put it back
struct TileChange
{
  int floor;
  int x, y;
  char tile;
};

struct Game
{
  std::string playerName;
  game::World world;
  std::map<std::string, pix::Sprite> sprites;      // by name, from sprites/
  std::map<std::string, game::Dialogue> dialogues; // by name, from dialogue/
  std::map<std::string, game::Scene> scenes;       // by name, from scenes/
  std::map<std::string, int> flags;                // the story's memory; unset flags are 0
  std::vector<Mission> missions;                   // in the order they were given
  Player player{0, 0.0f, 0.0f};
  std::vector<NpcState> npcs;       // characters on the current level
  std::map<int, int> visits;        // how many times each level has been entered
  std::map<std::string, int> talks; // how many times each character has been spoken to
  Destination checkpoint;           // where Purgatory's gates send you back to
  Transition transition;
  Banner banner;
  Toast toast;
  DialogueState dialogue;
  SceneState scene;
  Effects effects;
  int burden = 0;                       // added by the toll gate, lifted at Purgatory's summit
  std::vector<TileChange> tileChanges;  // every tile scenes have changed
  bool pendingAutosave = false;         // save once things settle after arriving
  bool logOpen = false;                 // mission log on screen
  bool inProgress = false;              // true once a game has been started
  bool returnToMenu = false;            // set by a scene's "menu" command
};
