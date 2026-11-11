#pragma once
#include <string>
#include <vector>

namespace game
{

	struct Dialogue
	{
		std::string speaker;
		std::vector<std::string> pages; // each page fits in the dialogue box
	};

	// Splits text into lines of at most maxChars characters, breaking between words
	std::vector<std::string> wrapText(const std::string &text, int maxChars);

	// Loads a dialogue file. Each line of text is one speech; speeches too long
	// for the box are split into several pages automatically.
	Dialogue loadDialogue(const std::string &path, int charsPerLine, int linesPerPage);

} // namespace game
