#include "game/scene.h"

#include <fstream>
#include <map>
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

    // Reads exactly as many whole numbers as there are outputs, or fails
    void readInts(const std::string &text, std::vector<int *> outs,
                  const std::string &path, int lineNo, const std::string &usage)
    {
      std::istringstream in(text);
      for (int *out : outs)
      {
        if (!(in >> *out))
          fail(path, lineNo, "expected " + usage);
      }
      std::string extra;
      if (in >> extra)
        fail(path, lineNo, "expected " + usage);
    }

    bool isCompare(const std::string &op)
    {
      return op == "==" || op == "!=" || op == "<" || op == "<=" || op == ">" || op == ">=";
    }

    // One word, no spaces: used for labels, flags and mission ids
    void requireWord(const std::string &word, const std::string &path, int lineNo, const std::string &usage)
    {
      if (word.empty() || word.find(' ') != std::string::npos)
        fail(path, lineNo, "expected " + usage);
    }

  } // namespace

  Scene loadScene(const std::string &path)
  {
    std::ifstream in(path);
    if (!in)
      throw std::runtime_error("Could not open scene file: " + path);

    Scene scene;
    std::map<std::string, int> labels; // label name -> index of its command
    std::string raw;
    int lineNo = 0;

    while (std::getline(in, raw))
    {
      ++lineNo;
      std::string line = trim(raw);
      if (line.empty() || line.rfind("//", 0) == 0)
        continue;

      size_t space = line.find(' ');
      std::string word = line.substr(0, space);
      std::string rest = (space == std::string::npos) ? "" : trim(line.substr(space + 1));

      SceneCommand c;
      c.line = lineNo;

      if (word == "say")
      {
        c.op = SceneOp::Say;
        size_t colon = rest.find(':');
        if (colon == std::string::npos)
          fail(path, lineNo, "expected 'say Speaker: text'");
        c.speaker = trim(rest.substr(0, colon));
        c.text = trim(rest.substr(colon + 1));
        if (c.text.empty())
          fail(path, lineNo, "'say' needs some text");
      }
      else if (word == "wait")
      {
        c.op = SceneOp::Wait;
        readInts(rest, {&c.frames}, path, lineNo, "'wait frames'");
      }
      else if (word == "fade")
      {
        c.op = SceneOp::Fade;
        readInts(rest, {&c.a, &c.b, &c.c, &c.d, &c.frames}, path, lineNo, "'fade r g b alpha frames'");
        for (int v : {c.a, c.b, c.c, c.d})
        {
          if (v < 0 || v > 255)
            fail(path, lineNo, "fade colors and alpha must be between 0 and 255");
        }
      }
      else if (word == "glow")
      {
        c.op = SceneOp::Glow;
        readInts(rest, {&c.a, &c.frames}, path, lineNo, "'glow radius frames'");
      }
      else if (word == "explore")
      {
        c.op = SceneOp::Explore;
        readInts(rest, {&c.a, &c.b}, path, lineNo, "'explore x y'");
      }
      else if (word == "choice")
      {
        c.op = SceneOp::Choice;
        std::istringstream parts(rest);
        std::string part;
        while (std::getline(parts, part, '|'))
        {
          size_t eq = part.find('=');
          if (eq == std::string::npos)
            fail(path, lineNo, "expected 'choice Text = label | Text = label'");

          SceneOption option;
          option.text = trim(part.substr(0, eq));
          option.label = trim(part.substr(eq + 1));
          if (option.text.empty() || option.label.empty())
            fail(path, lineNo, "each choice option needs text and a label");
          c.options.push_back(option);
        }
        if (c.options.size() < 2)
          fail(path, lineNo, "a choice needs at least two options");
      }
      else if (word == "label")
      {
        c.op = SceneOp::Label;
        c.text = rest;
        requireWord(rest, path, lineNo, "'label name' (one word)");
        if (labels.count(rest))
          fail(path, lineNo, "label '" + rest + "' is defined twice");
        labels[rest] = static_cast<int>(scene.commands.size());
      }
      else if (word == "goto")
      {
        c.op = SceneOp::Goto;
        c.text = rest;
        requireWord(rest, path, lineNo, "'goto label'");
      }
      else if (word == "caption")
      {
        c.op = SceneOp::Caption;
        c.text = rest;
        if (rest.empty())
          fail(path, lineNo, "'caption' needs some text");
      }
      else if (word == "settile")
      {
        c.op = SceneOp::SetTile;
        std::istringstream s(rest);
        std::string tile;
        if (!(s >> c.a >> c.b >> tile) || tile.size() != 1)
          fail(path, lineNo, "expected 'settile x y tile'");
        c.tile = tile[0];
      }
      else if (word == "set" || word == "add")
      {
        c.op = (word == "set") ? SceneOp::Set : SceneOp::Add;
        std::istringstream s(rest);
        std::string extra;
        if (!(s >> c.flag >> c.value) || (s >> extra))
          fail(path, lineNo, "expected '" + word + " flag number'");
      }
      else if (word == "if")
      {
        c.op = SceneOp::If;
        std::istringstream s(rest);
        std::vector<std::string> t;
        std::string token;
        while (s >> token)
          t.push_back(token);

        if (t.size() == 3 && t[1] == "goto")
        {
          // "if flag goto label": true when the flag isn't 0
          c.flag = t[0];
          c.compare = "!=";
          c.value = 0;
          c.text = t[2];
        }
        else if (t.size() == 5 && isCompare(t[1]) && t[3] == "goto")
        {
          c.flag = t[0];
          c.compare = t[1];
          try
          {
            c.value = std::stoi(t[2]);
          }
          catch (...)
          {
            fail(path, lineNo, "'" + t[2] + "' is not a whole number");
          }
          c.text = t[4];
        }
        else
        {
          fail(path, lineNo, "expected 'if flag goto label' or 'if flag >= number goto label'");
        }
      }
      else if (word == "mission" || word == "objective")
      {
        c.op = (word == "mission") ? SceneOp::Mission : SceneOp::Objective;
        size_t sp = rest.find(' ');
        if (sp == std::string::npos)
          fail(path, lineNo, "expected '" + word + " id text'");
        c.flag = rest.substr(0, sp);
        c.text = trim(rest.substr(sp + 1));
      }
      else if (word == "complete")
      {
        c.op = SceneOp::Complete;
        c.flag = rest;
        requireWord(rest, path, lineNo, "'complete id'");
      }
      else if (word == "banner" || word == "menu" || word == "end" || word == "die")
      {
        if (word == "banner")
          c.op = SceneOp::Banner;
        else if (word == "menu")
          c.op = SceneOp::Menu;
        else if (word == "end")
          c.op = SceneOp::End;
        else
          c.op = SceneOp::Die;
        if (!rest.empty())
          fail(path, lineNo, "'" + word + "' takes no arguments");
      }
      else
      {
        fail(path, lineNo, "unknown command '" + word + "'");
      }

      scene.commands.push_back(c);
    }

    // Connect every goto, if and choice to the label it jumps to
    for (SceneCommand &c : scene.commands)
    {
      if (c.op == SceneOp::Goto || c.op == SceneOp::If)
      {
        auto it = labels.find(c.text);
        if (it == labels.end())
          fail(path, c.line, "unknown label '" + c.text + "'");
        c.target = it->second;
      }
      else if (c.op == SceneOp::Choice)
      {
        for (SceneOption &option : c.options)
        {
          auto it = labels.find(option.label);
          if (it == labels.end())
            fail(path, c.line, "unknown label '" + option.label + "'");
          option.target = it->second;
        }
      }
    }

    return scene;
  }

} // namespace game
