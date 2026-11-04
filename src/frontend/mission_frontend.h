#pragma once
#include <filesystem>
#include "menu.h"
#include "mission_session.h"

namespace apache::frontend {
class mission_frontend {
  resources::data_store const &data;
  std::vector<resources::mission_briefing> definitions;
  std::vector<size_t> listed;
  std::unique_ptr<mission_session> prepared;
  std::optional<simulation::debrief_result> result;
  render::framebuffer map_image{640, 480};
  int flow{2}, zone{4}, selected{-1}, campaign_mission{0};
  unsigned int last_result{0};
  bool departing{false};
  void visible(std::string_view name, bool show);
  void list();
  void select(int row);
  void prepare(int id);
  void map();
  void subtitles(bool expanded);
  void campaign(bool new_campaign = false);
  void log_result(bool keep);
  bool activate(widget &object, int line);
public:
  menu interface;
  mission_options options;
  simulation::pilot_log pilot;
  std::function<void(simulation::pilot_log const &)> save;
  mission_frontend(resources::data_store const &data, mission_options options = {});
  std::unique_ptr<mission_session> takeoff();
  void complete(std::unique_ptr<mission_session> mission);
  void back();
};
} // namespace apache::frontend
