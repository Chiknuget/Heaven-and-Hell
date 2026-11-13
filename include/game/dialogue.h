#pragma once
#include <string>
#include <vector>

namespace game
{

	// One block of speech, used when its condition is true
	struct DialogueSection
	{
		std::string condition; // "first", "again", "burdened", "unburdened", "default" or ""
		std::vector<std::string> pages;
	};

	struct Dialogue
	{
		std::string speaker;
		std::vector<DialogueSection> sections;

		// The first section whose condition matches, or nullptr if none does
		const DialogueSection *choose(bool firstTime, bool burdened) const;
	};

	// Splits text into lines of at most maxChars characters, breaking between words
	std::vector<std::string> wrapText(const std::string &text, int maxChars);

	// Loads a dialogue file. Each line of text is one speech; speeches too long
	// for the box are split into several pages automatically. Throws
	// std::runtime_error with the file name and line number on mistakes.
	Dialogue loadDialogue(const std::string &path, int charsPerLine, int linesPerPage);

} // namespace game
