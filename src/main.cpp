#include "game/dialogue.h"
#include "game/level.h"
#include "gfx/font.h"
#include "gfx/framebuffer.h"
#include "gfx/iso.h"
#include "gfx/window.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <exception>
#include <string>
#include <thread>
#include <vector>

constexpr int GAME_W = 320;
constexpr int GAME_H = 180;
constexpr int START_SCALE = 4;

// Screen pixels moved per frame when walking sideways. Up/down moves half
// this, which matches the 2:1 slope of the iso grid. Keep it a whole number.
constexpr float SPEED_X = 1.0f;
constexpr float PLAYER_RADIUS = 0.2f; // half the width of the collision box, in tiles

constexpr int STAIRS_FADE = 20;		 // frames for each half of a stairs fade
constexpr int DEATH_FADE = 45;		 // slower fade when you die
constexpr int BANNER_FRAMES = 150; // how long the floor name stays up
constexpr int BANNER_FADE = 30;		 // banner fade in/out time, inside BANNER_FRAMES

constexpr int DIALOGUE_CHARS = 48; // characters per line in the dialogue box
constexpr int DIALOGUE_LINES = 3;	 // lines per page

struct Player
{
	int floor;	// index into world.levels
	float x, y; // position on that floor, in tiles
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
	bool valid = false;	 // there's something here
	bool usable = false; // E works; if false, text explains why not
	std::string text;		 // "ascend", or the reason it's blocked
	Destination dest;
	int burdenCost = 0; // Burden added for using it
};

enum class FadePhase
{
	None,
	FadeOut, // screen fading away, player frozen
	FadeIn	 // at the destination, screen coming back
};

struct Transition
{
	FadePhase phase = FadePhase::None;
	int frame = 0;
	int length = STAIRS_FADE;							// frames per half
	pix::Color color = pix::rgb(0, 0, 0); // what the screen fades to
	Destination dest;
};

struct Banner
{
	std::string title;
	std::string subtitle;
	int frame = BANNER_FRAMES; // starts "finished", so nothing shows
};

struct DialogueState
{
	bool active = false;
	game::Dialogue script;
	int page = 0;
	int shown = 0; // letters revealed so far on this page
	int tick = 0;	 // frame counter, for the blinking arrow
};

enum class ActorKind
{
	Player,
	Guide
};

// Anything standing in the world that needs depth sorting
struct Actor
{
	ActorKind kind;
	float x, y;
};

struct Game
{
	game::World world;
	game::Dialogue guideScript;
	Player player{0, 0.0f, 0.0f};
	Destination checkpoint; // where Purgatory's gates send you back to
	Transition transition;
	Banner banner;
	DialogueState dialogue;
	int burden = 0;						 // added by the toll gate, lifted at Purgatory's summit
	bool metGuide = false;		 // the guide only ever appears on your first death
	bool guidePresent = false; // the guide is waiting in Ante-Purgatory right now
	bool inProgress = false;	 // true once a game has been started
};

// ===================================================================
// Drawing helpers
// ===================================================================

// Blend from color a to color b; t goes from 0 to max
pix::Color mix(pix::Color a, pix::Color b, int t, int max)
{
	auto channel = [&](int shift)
	{
		int ca = (a >> shift) & 0xFF;
		int cb = (b >> shift) & 0xFF;
		return static_cast<std::uint8_t>(ca + (cb - ca) * t / max);
	};
	return pix::rgb(channel(16), channel(8), channel(0));
}

void drawGradient(pix::Framebuffer &fb, pix::Color top, pix::Color bottom)
{
	for (int y = 0; y < GAME_H; ++y)
	{
		fb.fillRect(0, y, GAME_W, 1, mix(top, bottom, y, GAME_H - 1));
	}
}

// Text with a drop shadow one font-pixel down and to the right
int drawShadowedText(pix::Framebuffer &fb, int x, int y, const std::string &text,
										 pix::Color color, pix::Color shadow, int scale = 1)
{
	pix::drawText(fb, x + scale, y + scale, text, shadow, scale);
	return pix::drawText(fb, x, y, text, color, scale);
}

// (fx, fy) is where the actor's feet touch the ground
void drawActor(pix::Framebuffer &fb, ActorKind kind, int fx, int fy)
{
	fb.fillRect(fx - 4, fy - 1, 8, 2, pix::rgb(40, 30, 40)); // shadow

	switch (kind)
	{
	case ActorKind::Player:
		fb.fillRect(fx - 3, fy - 10, 6, 10, pix::rgb(60, 120, 200)); // robe
		fb.fillRect(fx - 2, fy - 14, 4, 4, pix::rgb(240, 200, 170)); // head
		break;

	case ActorKind::Guide:
		fb.fillRect(fx - 3, fy - 12, 6, 12, pix::rgb(236, 234, 244)); // white robe
		fb.fillRect(fx - 3, fy - 7, 6, 1, pix::rgb(220, 180, 90));		// gold sash
		fb.fillRect(fx - 2, fy - 16, 4, 4, pix::rgb(240, 210, 180));	// head
		fb.fillRect(fx - 3, fy - 19, 6, 1, pix::rgb(255, 220, 100));	// halo
		break;
	}
}

// ===================================================================
// Movement and collision
// ===================================================================

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

// Moves one axis at a time. If an axis is blocked, the player is pushed
// right up against the wall, which lets them slide along it on the other axis.
void moveWithCollision(const game::Level &level, float &px, float &py, float dx, float dy)
{
	float nx = px + dx;
	if (!boxBlocked(level, nx, py))
		px = nx;
	else if (dx > 0)
		px = std::floor(nx + PLAYER_RADIUS) - PLAYER_RADIUS - 0.001f;
	else if (dx < 0)
		px = std::floor(nx - PLAYER_RADIUS) + 1.0f + PLAYER_RADIUS + 0.001f;

	float ny = py + dy;
	if (!boxBlocked(level, px, ny))
		py = ny;
	else if (dy > 0)
		py = std::floor(ny + PLAYER_RADIUS) - PLAYER_RADIUS - 0.001f;
	else if (dy < 0)
		py = std::floor(ny - PLAYER_RADIUS) + 1.0f + PLAYER_RADIUS + 0.001f;
}

// ===================================================================
// Stairs, gates and spawn points
// ===================================================================

Destination spawnOf(const game::World &w, int floor)
{
	const game::Level &l = w.levels[floor];
	return Destination{floor, l.spawnX + 0.5f, l.spawnY + 0.5f};
}

// The Nth '^' on a level leads to the Nth 'v' on the level above, and vice
// versa, within the same stack. Gates lead back to the checkpoint.
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

// A small box in the bottom-left corner, with the middle part highlighted
void drawPromptBox(pix::Framebuffer &fb, const std::string &before,
									 const std::string &highlight, const std::string &after)
{
	const int pad = 4;
	int boxW = pix::textWidth(before + highlight + after) + pad * 2;
	int boxH = pix::GLYPH_H + pad * 2 - 1;
	int boxX = 4;
	int boxY = GAME_H - 4 - boxH;

	fb.fillRect(boxX, boxY, boxW, boxH, pix::rgb(200, 190, 210));							 // border
	fb.fillRect(boxX + 1, boxY + 1, boxW - 2, boxH - 2, pix::rgb(24, 20, 32)); // background

	int x = boxX + pad;
	int y = boxY + pad;
	x = pix::drawText(fb, x, y, before, pix::rgb(240, 240, 240));
	x = pix::drawText(fb, x, y, highlight, pix::rgb(255, 210, 80));
	pix::drawText(fb, x, y, after, pix::rgb(240, 240, 240));
}

// "Press E to <verb>"
void drawPrompt(pix::Framebuffer &fb, const std::string &verb)
{
	drawPromptBox(fb, "Press ", "E", " to " + verb);
}

// A message with no key to press
void drawNotice(pix::Framebuffer &fb, const std::string &text)
{
	drawPromptBox(fb, text, "", "");
}

// Burden counter in the top-right corner
void drawBurden(pix::Framebuffer &fb, int burden)
{
	std::string text = "Burden: " + std::to_string(burden);
	int w = pix::textWidth(text) + 8;
	int h = pix::GLYPH_H + 7;
	int x = GAME_W - 4 - w;
	int y = 4;

	fb.fillRect(x, y, w, h, pix::rgb(150, 60, 70));
	fb.fillRect(x + 1, y + 1, w - 2, h - 2, pix::rgb(30, 14, 18));
	pix::drawText(fb, x + 4, y + 4, text, pix::rgb(230, 170, 170));
}

// ===================================================================
// Transitions and banners
// ===================================================================

void showBanner(Banner &b, const std::string &title, const std::string &subtitle)
{
	b.title = title;
	b.subtitle = subtitle;
	b.frame = 0;
}

// How strongly the fade color covers the screen: 0 = not at all, 255 = fully
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

// The level's name across the top of the screen, fading in and out
void drawBanner(pix::Framebuffer &fb, const Banner &b)
{
	if (b.frame >= BANNER_FRAMES)
		return;

	int a = 255;
	if (b.frame < BANNER_FADE)
		a = b.frame * 255 / BANNER_FADE;
	else if (b.frame > BANNER_FRAMES - BANNER_FADE)
		a = (BANNER_FRAMES - b.frame) * 255 / BANNER_FADE;

	const int scale = 2;
	const int bandY = 20;
	const bool hasSubtitle = !b.subtitle.empty();
	const int bandH = pix::GLYPH_H * scale + 12 + (hasSubtitle ? 11 : 0);

	// See-through dark band with thin gold lines above and below
	fb.fillRect(0, bandY, GAME_W, bandH, pix::rgba(0, 0, 0, a * 140 / 255));
	fb.fillRect(0, bandY, GAME_W, 1, pix::rgba(255, 220, 150, a * 180 / 255));
	fb.fillRect(0, bandY + bandH - 1, GAME_W, 1, pix::rgba(255, 220, 150, a * 180 / 255));

	int titleX = (GAME_W - pix::textWidth(b.title, scale)) / 2;
	int titleY = bandY + 7;
	drawShadowedText(fb, titleX, titleY, b.title, pix::rgba(255, 240, 210, a), pix::rgba(0, 0, 0, a), scale);

	if (hasSubtitle)
	{
		int subX = (GAME_W - pix::textWidth(b.subtitle)) / 2;
		int subY = titleY + pix::GLYPH_H * scale + 3;
		drawShadowedText(fb, subX, subY, b.subtitle, pix::rgba(220, 200, 170, a), pix::rgba(0, 0, 0, a));
	}
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

// ===================================================================
// Dialogue
// ===================================================================

// Number of letters on a page once it's wrapped into lines
int pageLength(const std::string &page)
{
	int n = 0;
	for (const std::string &line : game::wrapText(page, DIALOGUE_CHARS))
		n += static_cast<int>(line.size());
	return n;
}

void startDialogue(DialogueState &d, const game::Dialogue &script)
{
	if (script.pages.empty())
		return;

	d.active = true;
	d.script = script;
	d.page = 0;
	d.shown = 0;
	d.tick = 0;
}

void updateDialogue(const pix::Window &window, DialogueState &d)
{
	using pix::Key;
	++d.tick;

	int length = pageLength(d.script.pages[d.page]);
	if (d.shown < length)
		++d.shown; // typewriter: one letter per frame

	bool next = window.wasKeyPressed(Key::E) || window.wasKeyPressed(Key::Enter) || window.wasKeyPressed(Key::Space);
	if (!next)
		return;

	// The first press finishes the page; the next one turns it
	if (d.shown < length)
	{
		d.shown = length;
		return;
	}

	++d.page;
	d.shown = 0;
	if (d.page >= static_cast<int>(d.script.pages.size()))
		d.active = false;
}

void drawDialogue(pix::Framebuffer &fb, const DialogueState &d)
{
	if (!d.active)
		return;

	const pix::Color border = pix::rgb(200, 190, 210);
	const pix::Color background = pix::rgb(24, 20, 32);
	const pix::Color textColor = pix::rgb(240, 240, 240);
	const pix::Color gold = pix::rgb(255, 210, 80);

	const int pad = 6;
	const int lineH = 10;
	const int boxX = 8;
	const int boxW = GAME_W - 16;
	const int boxH = DIALOGUE_LINES * lineH - 2 + pad * 2;
	const int boxY = GAME_H - 6 - boxH;

	fb.fillRect(boxX, boxY, boxW, boxH, border);
	fb.fillRect(boxX + 1, boxY + 1, boxW - 2, boxH - 2, background);

	// Name tab on top of the box
	if (!d.script.speaker.empty())
	{
		int tabW = pix::textWidth(d.script.speaker) + 8;
		fb.fillRect(boxX, boxY - 11, tabW, 12, border);
		fb.fillRect(boxX + 1, boxY - 10, tabW - 2, 11, background); // joins the tab to the box
		pix::drawText(fb, boxX + 4, boxY - 8, d.script.speaker, gold);
	}

	// Text, revealed letter by letter
	const std::string &page = d.script.pages[d.page];
	std::vector<std::string> lines = game::wrapText(page, DIALOGUE_CHARS);
	int remaining = d.shown;

	for (int i = 0; i < static_cast<int>(lines.size()) && remaining > 0; ++i)
	{
		int count = std::min(remaining, static_cast<int>(lines[i].size()));
		pix::drawText(fb, boxX + pad, boxY + pad + i * lineH, lines[i].substr(0, count), textColor);
		remaining -= static_cast<int>(lines[i].size());
	}

	// Blinking arrow once the page is fully shown
	if (d.shown >= pageLength(page) && (d.tick / 20) % 2 == 0)
	{
		pix::drawText(fb, boxX + boxW - pad - pix::GLYPH_W, boxY + boxH - pad - 6, "v", gold);
	}
}

// ===================================================================
// The game
// ===================================================================

// Puts the player at a destination and handles everything that
// happens on arrival
void arrive(Game &g, const Destination &d)
{
	const game::World &w = g.world;
	g.player = Player{d.floor, d.x, d.y};

	const game::Level &level = w.levels[d.floor];
	showBanner(g.banner, level.name, level.subtitle);

	if (!w.inPurgatory(d.floor))
	{
		g.checkpoint = d;				// arriving on a main floor saves your place
		g.guidePresent = false; // the guide stays behind when you leave Purgatory
	}

	if (d.floor == w.purgatoryTop)
		g.burden = 0; // the summit lifts every burden

	if (d.floor == w.purgatoryBottom && !g.metGuide)
	{
		g.metGuide = true;
		g.guidePresent = true;
		startDialogue(g.dialogue, g.guideScript);
	}
}

void startNewGame(Game &g)
{
	g = Game{};
	g.world = game::loadWorld("levels/world.txt"); // reloaded so map edits show up
	g.guideScript = game::loadDialogue("levels/guide.txt", DIALOGUE_CHARS, DIALOGUE_LINES);
	arrive(g, spawnOf(g.world, game::HEAVEN));
	g.inProgress = true;
}

void updateGame(const pix::Window &window, Game &g)
{
	using pix::Key;
	Transition &t = g.transition;

	if (g.banner.frame < BANNER_FRAMES)
		++g.banner.frame;

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

	const game::Level &level = g.world.levels[g.player.floor];

	int ix = (window.isKeyDown(Key::D) || window.isKeyDown(Key::Right)) - (window.isKeyDown(Key::A) || window.isKeyDown(Key::Left));
	int iy = (window.isKeyDown(Key::S) || window.isKeyDown(Key::Down)) - (window.isKeyDown(Key::W) || window.isKeyDown(Key::Up));

	// Velocity in screen pixels per frame
	float vx = ix * SPEED_X;
	float vy = iy * SPEED_X * 0.5f;

	// Screen pixels -> world tiles
	float dx = vx / pix::TILE_W + vy / pix::TILE_H;
	float dy = vy / pix::TILE_H - vx / pix::TILE_W;

	if (dx != 0.0f || dy != 0.0f)
	{
		moveWithCollision(level, g.player.x, g.player.y, dx, dy);
	}

	// Nothing else happens until the current transition has finished
	if (t.phase != FadePhase::None)
		return;

	// Death: the middle of the player is over something deadly
	char under = level.at(static_cast<int>(std::floor(g.player.x)), static_cast<int>(std::floor(g.player.y)));
	if (game::tileType(under).deadly)
	{
		Destination dest = (g.world.purgatoryBottom >= 0) ? spawnOf(g.world, g.world.purgatoryBottom) : g.checkpoint;
		startTransition(g, dest, DEATH_FADE, pix::rgb(255, 255, 255));
		return;
	}

	// Stairs and gates
	ExitInfo ex = exitUnderPlayer(g);
	if (ex.valid && ex.usable && window.wasKeyPressed(Key::E))
	{
		g.burden += ex.burdenCost;
		startTransition(g, ex.dest, STAIRS_FADE, pix::rgb(0, 0, 0));
	}
}

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
		drawActor(fb, a.kind, feetX, feetY);

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
	const game::World &w = g.world;
	const game::Level &level = w.levels[g.player.floor];

	std::vector<Actor> actors;
	actors.push_back({ActorKind::Player, g.player.x, g.player.y});
	if (g.guidePresent && g.player.floor == w.purgatoryBottom && level.guideX >= 0)
	{
		actors.push_back({ActorKind::Guide, level.guideX + 0.5f, level.guideY + 0.5f});
	}

	drawLevel(fb, level, g.player, actors);

	bool calm = (g.transition.phase == FadePhase::None);

	if (calm && !g.dialogue.active)
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

	if (g.burden > 0)
		drawBurden(fb, g.burden);

	drawBanner(fb, g.banner);

	if (calm)
		drawDialogue(fb, g.dialogue);

	// Fade overlay goes on top of everything
	int fade = fadeAmount(g.transition);
	if (fade > 0)
	{
		pix::Color overlay = (g.transition.color & 0x00FFFFFFu) | (static_cast<pix::Color>(fade) << 24);
		fb.fillRect(0, 0, GAME_W, GAME_H, overlay);
	}
}

// ===================================================================
// Main menu
// ===================================================================

enum class Screen
{
	Menu,
	Playing
};

enum class MenuAction
{
	Continue,
	NewGame,
	Quit
};

struct MenuItem
{
	std::string label;
	MenuAction action;
};

std::vector<MenuItem> menuItems(const Game &g)
{
	std::vector<MenuItem> items;
	if (g.inProgress)
		items.push_back({"Continue", MenuAction::Continue});
	items.push_back({"New Game", MenuAction::NewGame});
	items.push_back({"Quit", MenuAction::Quit});
	return items;
}

void drawMenu(pix::Framebuffer &fb, const std::vector<MenuItem> &items, int selected)
{
	// Heaven at the top fading down into hell
	drawGradient(fb, pix::rgb(255, 236, 180), pix::rgb(70, 8, 8));

	// Title
	const std::string title = "Heaven & Hell";
	const int titleScale = 3;
	int titleX = (GAME_W - pix::textWidth(title, titleScale)) / 2;
	drawShadowedText(fb, titleX, 26, title, pix::rgb(70, 16, 24), pix::rgb(200, 150, 90), titleScale);

	// Menu items, centered, with an arrow next to the selected one
	const int itemScale = 2;
	const int firstY = 82;
	const int spacing = 22;

	for (int i = 0; i < static_cast<int>(items.size()); ++i)
	{
		const std::string &label = items[i].label;
		int x = (GAME_W - pix::textWidth(label, itemScale)) / 2;
		int y = firstY + i * spacing;
		bool isSelected = (i == selected);

		pix::Color color = isSelected ? pix::rgb(255, 220, 110) : pix::rgb(235, 225, 215);
		drawShadowedText(fb, x, y, label, color, pix::rgb(30, 10, 10), itemScale);

		if (isSelected)
		{
			int arrowX = x - pix::GLYPH_ADVANCE * itemScale * 2;
			drawShadowedText(fb, arrowX, y, ">", color, pix::rgb(30, 10, 10), itemScale);
		}
	}

	// Controls hint at the bottom
	const std::string hint = "W/S: choose   Enter: select";
	int hintX = (GAME_W - pix::textWidth(hint)) / 2;
	drawShadowedText(fb, hintX, GAME_H - 14, hint, pix::rgb(200, 170, 160), pix::rgb(20, 4, 4));
}

// ===================================================================
// Main loop
// ===================================================================

int main()
{
	try
	{
		pix::Window window("Heaven & Hell", GAME_W * START_SCALE, GAME_H * START_SCALE);
		pix::Framebuffer fb(GAME_W, GAME_H);

		Game game;
		Screen screen = Screen::Menu;
		int selected = 0;
		bool running = true;

		using Clock = std::chrono::steady_clock;
		const auto frameTime = std::chrono::microseconds(1'000'000 / 60);
		auto nextFrame = Clock::now();

		while (running && window.isOpen())
		{
			window.pollEvents();
			using pix::Key;

			if (screen == Screen::Menu)
			{
				std::vector<MenuItem> items = menuItems(game);
				int count = static_cast<int>(items.size());
				if (selected >= count)
					selected = 0;

				if (window.wasKeyPressed(Key::W) || window.wasKeyPressed(Key::Up))
					selected = (selected + count - 1) % count;
				if (window.wasKeyPressed(Key::S) || window.wasKeyPressed(Key::Down))
					selected = (selected + 1) % count;

				bool confirm = window.wasKeyPressed(Key::Enter) || window.wasKeyPressed(Key::Space) || window.wasKeyPressed(Key::E);

				if (game.inProgress && window.wasKeyPressed(Key::Escape))
				{
					screen = Screen::Playing; // Escape again goes straight back in
				}
				else if (confirm)
				{
					switch (items[selected].action)
					{
					case MenuAction::Continue:
						screen = Screen::Playing;
						break;
					case MenuAction::NewGame:
						startNewGame(game);
						screen = Screen::Playing;
						break;
					case MenuAction::Quit:
						running = false;
						break;
					}
				}

				drawMenu(fb, items, selected);
			}
			else
			{
				if (window.wasKeyPressed(Key::Escape))
				{
					screen = Screen::Menu;
					selected = 0; // lands on Continue
				}
				else
				{
					updateGame(window, game);
				}

				drawGame(fb, game);
			}

			window.present(fb);

			nextFrame += frameTime;
			std::this_thread::sleep_until(nextFrame);
		}
	}
	catch (const std::exception &e)
	{
		std::fprintf(stderr, "Error: %s\n", e.what());
		return 1;
	}

	return 0;
}
