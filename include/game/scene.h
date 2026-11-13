#pragma once
#include <string>
#include <vector>

namespace game
{

  enum class SceneOp
  {
    Say,
    Wait,
    Fade,
    Glow,
    Explore,
    Choice,
    Label,
    Goto,
    Caption,
    SetTile,
    Banner,
    Menu,
    End,
    Set,
    Add,
    If,
    Mission,
    Objective,
    Complete,
    Die
  };

  struct SceneOption
  {
    std::string text;  // shown in the choice box
    std::string label; // where choosing it jumps to
    int target = -1;   // index of that label's command, filled in after loading
  };

  struct SceneCommand
  {
    SceneOp op = SceneOp::End;
    std::string speaker;              // say
    std::string text;                 // say, caption, objective text, mission title; the label for label, goto and if
    std::string flag;                 // set, add, if; the mission id for mission, objective and complete
    std::string compare;              // if: == != < <= > >=
    int value = 0;                    // set, add, if
    int a = 0, b = 0, c = 0, d = 0;   // numbers: fade r g b alpha, glow radius, explore/settile x y
    int frames = 0;                   // wait, fade, glow
    char tile = ' ';                  // settile
    int target = -1;                  // goto and if: index of the label's command
    std::vector<SceneOption> options; // choice
    int line = 0;                     // line in the scene file, for error messages
  };

  struct Scene
  {
    std::vector<SceneCommand> commands;
  };

  // Loads a scene script. Throws std::runtime_error with the file name and
  // line number on mistakes, including labels that don't exist.
  Scene loadScene(const std::string &path);

} // namespace game
