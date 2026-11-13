#include "play/missions.h"

void showBanner(Banner &b, const std::string &title, const std::string &subtitle)
{
  b.title = title;
  b.subtitle = subtitle;
  b.frame = 0;
}

void showToast(Game &g, const std::string &text)
{
  g.toast.text = text;
  g.toast.frame = 0;
}

Mission *findMission(Game &g, const std::string &id)
{
  for (Mission &m : g.missions)
  {
    if (m.id == id)
      return &m;
  }
  return nullptr;
}

void startMission(Game &g, const std::string &id, const std::string &title)
{
  if (findMission(g, id))
    return; // already given
  g.missions.push_back(Mission{id, title, "", false});
  showToast(g, "New mission: " + title + "  (Tab: log)");
}

void setObjective(Game &g, const std::string &id, const std::string &text)
{
  if (Mission *m = findMission(g, id))
    m->objective = text;
}

void completeMission(Game &g, const std::string &id)
{
  Mission *m = findMission(g, id);
  if (!m || m->done)
    return;
  m->done = true;
  showToast(g, "Mission complete: " + m->title);
}
