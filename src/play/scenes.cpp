#include "play/scenes.h"

#include "play/missions.h"
#include "play/talk.h"
#include "play/text.h"
#include "play/ui.h"
#include "play/world.h"

#include <algorithm>

void startScene(Game &g, const std::string &name)
{
  auto it = g.scenes.find(name);
  if (it == g.scenes.end())
    return;

  g.scene = SceneState{};
  g.scene.scene = &it->second;
  g.effects = Effects{};
}

bool updateScene(const pix::Window &window, Game &g)
{
  using pix::Key;
  SceneState &s = g.scene;
  Effects &fx = g.effects;
  ++s.tick;

  // A fade or glow in progress finishes before anything else happens
  if (fx.tween != Tween::None)
  {
    ++fx.frame;
    if (fx.tween == Tween::Fade)
    {
      fx.fadeColor = mix(fx.fromColor, fx.toColor, fx.frame, fx.length);
      fx.fadeAlpha = fx.from + (fx.to - fx.from) * fx.frame / fx.length;
    }
    else
    {
      fx.glowRadius = fx.from + (fx.to - fx.from) * fx.frame / fx.length;
    }

    if (fx.frame >= fx.length)
      fx.tween = Tween::None;
    return false;
  }

  if (s.waitFrames > 0)
  {
    --s.waitFrames;
    return false;
  }

  bool confirm = window.wasKeyPressed(Key::E) || window.wasKeyPressed(Key::Enter) || window.wasKeyPressed(Key::Space);

  if (s.captioning)
  {
    if (!confirm)
      return false;
    s.captioning = false;
  }

  if (s.choice)
  {
    int count = static_cast<int>(s.choice->options.size());
    if (window.wasKeyPressed(Key::W) || window.wasKeyPressed(Key::Up))
      s.choiceSelected = (s.choiceSelected + count - 1) % count;
    if (window.wasKeyPressed(Key::S) || window.wasKeyPressed(Key::Down))
      s.choiceSelected = (s.choiceSelected + 1) % count;

    if (!confirm)
      return false;

    s.pc = s.choice->options[s.choiceSelected].target;
    s.choice = nullptr;
  }

  if (s.exploring)
  {
    float dx = g.player.x - s.exploreX;
    float dy = g.player.y - s.exploreY;
    if (dx * dx + dy * dy > EXPLORE_REACH * EXPLORE_REACH)
      return true; // keep walking
    s.exploring = false;
  }

  // Run commands until one of them has to wait for something
  for (int guard = 0; guard < 1000; ++guard)
  {
    if (s.pc >= static_cast<int>(s.scene->commands.size()))
    {
      s.scene = nullptr; // ran off the end: same as "end"
      return true;
    }

    const game::SceneCommand &c = s.scene->commands[s.pc++];

    switch (c.op)
    {
    case game::SceneOp::Say:
      startDialogue(g.dialogue, c.speaker, {c.text}, g.playerName, g.flags);
      return false;

    case game::SceneOp::Wait:
      s.waitFrames = c.frames;
      return false;

    case game::SceneOp::Fade:
      fx.tween = Tween::Fade;
      fx.frame = 0;
      fx.length = std::max(1, c.frames);
      fx.fromColor = fx.fadeColor;
      fx.toColor = pix::rgb(c.a, c.b, c.c);
      fx.from = fx.fadeAlpha;
      fx.to = c.d;
      return false;

    case game::SceneOp::Glow:
      fx.tween = Tween::Glow;
      fx.frame = 0;
      fx.length = std::max(1, c.frames);
      fx.from = fx.glowRadius;
      fx.to = c.a;
      return false;

    case game::SceneOp::Explore:
      s.exploring = true;
      s.exploreX = c.a + 0.5f;
      s.exploreY = c.b + 0.5f;
      return true;

    case game::SceneOp::Choice:
      s.choice = &c;
      s.choiceSelected = 0;
      return false;

    case game::SceneOp::Label:
      break; // just a marker

    case game::SceneOp::Goto:
      s.pc = c.target;
      break;

    case game::SceneOp::If:
      if (compareValues(flagValue(g, c.flag), c.compare, c.value))
        s.pc = c.target;
      break;

    case game::SceneOp::Set:
      g.flags[c.flag] = c.value;
      refreshNpcs(g);
      break;

    case game::SceneOp::Add:
      g.flags[c.flag] += c.value;
      refreshNpcs(g);
      break;

    case game::SceneOp::Mission:
      startMission(g, c.flag, c.text);
      break;

    case game::SceneOp::Objective:
      setObjective(g, c.flag, c.text);
      break;

    case game::SceneOp::Complete:
      completeMission(g, c.flag);
      break;

    case game::SceneOp::Caption:
      s.captioning = true;
      s.caption = fillText(c.text, g.playerName, g.flags);
      return false;

    case game::SceneOp::SetTile:
      setTile(g, g.player.floor, c.a, c.b, c.tile);
      break;

    case game::SceneOp::Banner:
    {
      const game::Level &level = g.world.levels[g.player.floor];
      showBanner(g.banner, level.name, level.subtitle);
      break;
    }

    case game::SceneOp::Die:
      s.scene = nullptr;
      startTransition(g, deathDestination(g), DEATH_FADE, pix::rgb(255, 255, 255));
      return false;

    case game::SceneOp::Menu:
      s.scene = nullptr;
      g.inProgress = false;
      g.returnToMenu = true;
      return false;

    case game::SceneOp::End:
      s.scene = nullptr;
      return true;
    }
  }

  s.scene = nullptr; // a goto loop with nothing that waits: stop the scene
  return true;
}
