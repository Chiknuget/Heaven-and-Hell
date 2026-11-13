#include "play/text.h"

int flagValue(const Game &g, const std::string &flag)
{
  auto it = g.flags.find(flag);
  return it == g.flags.end() ? 0 : it->second;
}

std::string fillText(const std::string &text, const std::string &name, const std::map<std::string, int> &flags)
{
  std::string out;
  size_t i = 0;

  while (i < text.size())
  {
    size_t open = text.find('{', i);
    size_t close = (open == std::string::npos) ? std::string::npos : text.find('}', open);
    if (close == std::string::npos)
    {
      out += text.substr(i);
      break;
    }

    out += text.substr(i, open - i);
    std::string key = text.substr(open + 1, close - open - 1);
    if (key == "name")
    {
      out += name;
    }
    else
    {
      auto it = flags.find(key);
      out += std::to_string(it == flags.end() ? 0 : it->second);
    }
    i = close + 1;
  }
  return out;
}

bool compareValues(int a, const std::string &op, int b)
{
  if (op == "==")
    return a == b;
  if (op == "!=")
    return a != b;
  if (op == "<")
    return a < b;
  if (op == "<=")
    return a <= b;
  if (op == ">")
    return a > b;
  return a >= b; // ">="
}
