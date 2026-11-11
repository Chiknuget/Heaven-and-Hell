#include "game/dialogue.h"

#include <algorithm>
#include <fstream>
#include <sstream>
#include <stdexcept>

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

  } // namespace

  std::vector<std::string> wrapText(const std::string &text, int maxChars)
  {
    std::vector<std::string> lines;
    std::istringstream words(text);
    std::string word;
    std::string line;

    while (words >> word)
    {
      // A single word longer than a whole line gets chopped up
      while (static_cast<int>(word.size()) > maxChars)
      {
        if (!line.empty())
        {
          lines.push_back(line);
          line.clear();
        }
        lines.push_back(word.substr(0, maxChars));
        word = word.substr(maxChars);
      }

      if (line.empty())
      {
        line = word;
      }
      else if (static_cast<int>(line.size() + 1 + word.size()) <= maxChars)
      {
        line += " " + word;
      }
      else
      {
        lines.push_back(line);
        line = word;
      }
    }

    if (!line.empty())
      lines.push_back(line);
    return lines;
  }

  Dialogue loadDialogue(const std::string &path, int charsPerLine, int linesPerPage)
  {
    std::ifstream in(path);
    if (!in)
      throw std::runtime_error("Could not open dialogue file: " + path);

    Dialogue d;
    std::string raw;

    while (std::getline(in, raw))
    {
      std::string line = trim(raw);
      if (line.empty() || line.rfind("//", 0) == 0)
        continue;

      if (line.rfind("name:", 0) == 0)
      {
        d.speaker = trim(line.substr(5));
        continue;
      }

      // Group the wrapped lines into pages that fit the box
      std::vector<std::string> lines = wrapText(line, charsPerLine);
      for (size_t i = 0; i < lines.size(); i += linesPerPage)
      {
        std::string page;
        size_t end = std::min(lines.size(), i + linesPerPage);
        for (size_t j = i; j < end; ++j)
        {
          if (!page.empty())
            page += ' ';
          page += lines[j];
        }
        d.pages.push_back(page);
      }
    }

    return d;
  }

} // namespace game
