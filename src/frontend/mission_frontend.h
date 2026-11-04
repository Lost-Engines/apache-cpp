#pragma once
#include <filesystem>
#include "menu.h"
#include "pilot_menu.h"
#include "score_menu.h"
#include "preferences_menu.h"
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
  bool first_launch{false}, first_quick{false}, invincible_quick{false};
  void launch_quick();
  void count_run();
  std::optional<simulation::campaign_progress> transferred_campaign;
  void show_pilots(bool transfer_campaign);
  void show_awards(unsigned int mask);
  void apply_preferences();
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
  configuration::settings settings;
  preferences_menu preferences;
  configuration::profiles logs;
  pilot_menu pilots;
  score_menu scores;
  simulation::pilot_log &pilot() { return logs.current(); }
  void load_profiles(std::filesystem::path const &path);
  void startup();
  bool text_input(unsigned int value) { return scores.key(value) || pilots.key(value); }
  mission_frontend(resources::data_store const &data, mission_options options = {});
  std::unique_ptr<mission_session> takeoff();
  void complete(std::unique_ptr<mission_session> mission);
  void back();
};
} // namespace apache::frontend
