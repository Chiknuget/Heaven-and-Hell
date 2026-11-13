#include "gfx/font.h"
#include "gfx/framebuffer.h"
#include "gfx/window.h"
#include "play/render.h"
#include "play/save.h"
#include "play/state.h"
#include "play/ui.h"
#include "play/world.h"

#include <cctype>
#include <chrono>
#include <cstdio>
#include <exception>
#include <string>
#include <thread>
#include <vector>

// ===================================================================
// Main menu
// ===================================================================

enum class Screen
{
	Menu,
	NameEntry,
	Playing
};

enum class MenuAction
{
	Continue,
	Save,
	Load,
	NewGame,
	Quit
};

struct MenuItem
{
	std::string label;
	MenuAction action;
};

// A message shown under the menu for a few seconds
struct MenuMessage
{
	std::string text;
	int frames = 0;
};

std::vector<MenuItem> menuItems(const Game &g)
{
	std::vector<MenuItem> items;
	if (g.inProgress)
	{
		items.push_back({"Continue", MenuAction::Continue});
		items.push_back({"Save Game", MenuAction::Save});
	}
	if (saveExists())
		items.push_back({"Load Game", MenuAction::Load});
	items.push_back({"New Game", MenuAction::NewGame});
	items.push_back({"Quit", MenuAction::Quit});
	return items;
}

void drawMenu(pix::Framebuffer &fb, const std::vector<MenuItem> &items, int selected, const MenuMessage &message)
{
	// Heaven at the top fading down into hell
	drawGradient(fb, pix::rgb(255, 236, 180), pix::rgb(70, 8, 8));

	// Title
	const std::string title = "Heaven & Hell";
	const int titleScale = 3;
	int titleX = (GAME_W - pix::textWidth(title, titleScale)) / 2;
	drawShadowedText(fb, titleX, 22, title, pix::rgb(70, 16, 24), pix::rgb(200, 150, 90), titleScale);

	// Menu items, centered, with an arrow next to the selected one
	const int itemScale = 2;
	const int firstY = 64;
	const int spacing = 19;

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

	// A recent message, or the controls hint
	const std::string bottom = (message.frames > 0) ? message.text : "W/S: choose   Enter: select";
	pix::Color bottomColor = (message.frames > 0) ? pix::rgb(255, 230, 180) : pix::rgb(200, 170, 160);
	int bottomX = (GAME_W - pix::textWidth(bottom)) / 2;
	drawShadowedText(fb, bottomX, GAME_H - 14, bottom, bottomColor, pix::rgb(20, 4, 4));
}

// ===================================================================
// Name entry
// ===================================================================

struct NameEntry
{
	std::string name;
	int tick = 0;
};

void drawNameEntry(pix::Framebuffer &fb, const NameEntry &n)
{
	drawGradient(fb, pix::rgb(255, 236, 180), pix::rgb(70, 8, 8));

	const std::string question = "What is your name?";
	int qx = (GAME_W - pix::textWidth(question, 2)) / 2;
	drawShadowedText(fb, qx, 44, question, pix::rgb(70, 16, 24), pix::rgb(200, 150, 90), 2);

	// The name box, wide enough for the longest name
	const int scale = 2;
	const int boxW = MAX_NAME_LENGTH * pix::GLYPH_ADVANCE * scale + 16;
	const int boxH = pix::GLYPH_H * scale + 12;
	const int boxX = (GAME_W - boxW) / 2;
	const int boxY = 80;

	fb.fillRect(boxX, boxY, boxW, boxH, pix::rgb(200, 190, 210));
	fb.fillRect(boxX + 1, boxY + 1, boxW - 2, boxH - 2, pix::rgb(24, 20, 32));

	int x = pix::drawText(fb, boxX + 8, boxY + 6, n.name, pix::rgb(240, 240, 240), scale);

	// Blinking cursor
	if ((n.tick / 30) % 2 == 0 && static_cast<int>(n.name.size()) < MAX_NAME_LENGTH)
	{
		fb.fillRect(x, boxY + 6 + (pix::GLYPH_H - 1) * scale, pix::GLYPH_W * scale, scale, pix::rgb(255, 210, 80));
	}

	const std::string hint = "Enter: confirm   Esc: back";
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
		NameEntry nameEntry;
		MenuMessage message;
		Screen screen = Screen::Menu;
		int selected = 0;
		bool running = true;

		using Clock = std::chrono::steady_clock;
		const auto frameTime = std::chrono::microseconds(1'000'000 / 60);
		auto nextFrame = Clock::now();

		auto showMessage = [&](const std::string &text)
		{
			message.text = text;
			message.frames = 180;
		};

		while (running && window.isOpen())
		{
			window.pollEvents();
			using pix::Key;

			if (message.frames > 0)
				--message.frames;

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

					case MenuAction::Save:
						if (!canSave(game))
						{
							showMessage("You can't save in the middle of a scene.");
							break;
						}
						try
						{
							saveGame(game);
							showMessage("Game saved.");
						}
						catch (const std::exception &e)
						{
							std::fprintf(stderr, "Save failed: %s\n", e.what());
							showMessage("Could not save the game.");
						}
						break;

					case MenuAction::Load:
						try
						{
							loadGame(game);
							screen = Screen::Playing;
						}
						catch (const std::exception &e)
						{
							std::fprintf(stderr, "Load failed: %s\n", e.what());
							showMessage("The save could not be loaded.");
						}
						break;

					case MenuAction::NewGame:
						nameEntry = NameEntry{};
						screen = Screen::NameEntry;
						break;

					case MenuAction::Quit:
						running = false;
						break;
					}
				}

				drawMenu(fb, items, selected, message);
			}
			else if (screen == Screen::NameEntry)
			{
				++nameEntry.tick;

				for (char c : window.typedText())
				{
					bool allowed = std::isalnum(static_cast<unsigned char>(c)) || c == ' ' || c == '-' || c == '\'';
					if (!allowed || static_cast<int>(nameEntry.name.size()) >= MAX_NAME_LENGTH)
						continue;
					if (c == ' ' && (nameEntry.name.empty() || nameEntry.name.back() == ' '))
						continue; // no leading or double spaces
					nameEntry.name += c;
				}

				if (window.wasKeyPressed(Key::Backspace) && !nameEntry.name.empty())
					nameEntry.name.pop_back();

				if (window.wasKeyPressed(Key::Escape))
				{
					screen = Screen::Menu;
				}
				else if (window.wasKeyPressed(Key::Enter))
				{
					std::string name = nameEntry.name;
					while (!name.empty() && name.back() == ' ')
						name.pop_back();

					if (!name.empty())
					{
						startNewGame(game, name);
						screen = Screen::Playing;
					}
				}

				drawNameEntry(fb, nameEntry);
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

				// A scene ended the game (like refusing in the prologue)
				if (game.returnToMenu)
				{
					game.returnToMenu = false;
					screen = Screen::Menu;
					selected = 0;
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
