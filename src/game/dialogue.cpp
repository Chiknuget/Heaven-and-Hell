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

    [[noreturn]] void fail(const std::string &path, int lineNo, const std::string &msg)
    {
      throw std::runtime_error(path + ":" + std::to_string(lineNo) + ": " + msg);
    }

    bool isKnownCondition(const std::string &c)
    {
      return c == "first" || c == "again" || c == "burdened" || c == "unburdened" || c == "default";
    }

  } // namespace

  const DialogueSection *Dialogue::choose(bool firstTime, bool burdened) const
  {
    for (const DialogueSection &s : sections)
    {
      const std::string &c = s.condition;
      bool match = c.empty() || c == "default" ||
                   (c == "first" && firstTime) ||
                   (c == "again" && !firstTime) ||
                   (c == "burdened" && burdened) ||
                   (c == "unburdened" && !burdened);
      if (match)
        return &s;
    }
    return nullptr;
  }

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

  std::vector<std::string> paginate(const std::string &text, int charsPerLine, int linesPerPage)
  {
    std::vector<std::string> pages;
    std::vector<std::string> lines = wrapText(text, charsPerLine);

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
      pages.push_back(page);
    }
    return pages;
  }

  Dialogue loadDialogue(const std::string &path)
  {
    std::ifstream in(path);
    if (!in)
      throw std::runtime_error("Could not open dialogue file: " + path);

    Dialogue d;
    std::string raw;
    int lineNo = 0;

    while (std::getline(in, raw))
    {
      ++lineNo;
      std::string line = trim(raw);
      if (line.empty() || line.rfind("//", 0) == 0)
        continue;

      if (line.rfind("name:", 0) == 0)
      {
        d.speaker = trim(line.substr(5));
        continue;
      }

      // [condition] starts a new section
      if (line.front() == '[')
      {
        if (line.back() != ']')
          fail(path, lineNo, "section header is missing its ']'");
        std::string condition = trim(line.substr(1, line.size() - 2));
        if (!isKnownCondition(condition))
          fail(path, lineNo, "unknown condition '" + condition + "'");
        d.sections.push_back({condition, {}});
        continue;
      }

      // Lines before any section header always apply
      if (d.sections.empty())
        d.sections.push_back({"", {}});
      d.sections.back().lines.push_back(line);
    }

    return d;
  }

} // namespace game
