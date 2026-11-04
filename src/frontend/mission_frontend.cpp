#include "mission_frontend.h"
#include <algorithm>
#include <chrono>
#include <stdexcept>
#include "render/menu_font.h"

namespace apache::frontend {
namespace {
std::string text(resources::mission_briefing const &mission, std::string_view const tag) {
  std::string result;
  bool active{false};
  for(auto const &section : mission.text) {
    if(section.tag != "Para") active = section.tag == tag;
    else if(active) result += "\n\n";
    if(active) for(auto const &line : section.lines) {
      if(!result.empty() && result.back() != ' ' && result.back() != '\n') result += ' ';
      result += line;
    }
  }
  return result;
}
uint32_t hours_now() {
  return static_cast<uint32_t>(std::chrono::duration_cast<std::chrono::hours>(std::chrono::system_clock::now().time_since_epoch()).count());
}
std::string zone_name(int const zone) {
  return std::array<std::string, 4>{"yemen", "korea", "cyprus", "fhood"}.at(static_cast<size_t>(zone - 1));
}
}
mission_frontend::mission_frontend(resources::data_store const &data, mission_options const settings)
  : data{data}, definitions{resources::read_mission_briefings(data.read("MISSDATA/MISSIONS.BLK"))}, interface{data}, options{settings} {
  pilot.name = "de Fault"; pilot.rank = 6; pilot.protected_profile = 1; pilot.status_hour = hours_now();
  interface.link_scroll("_map_briefing_subtitle_text", "_map_briefing_subtitle_scrollbar");
  interface.link_scroll("_situation_text", "_situation_scroll_bar");
  interface.activated = [this](widget &object, int line){ return activate(object, line); };
  interface.paint = [this](render::framebuffer &frame, widget const &object){
    if(object.type == 5 && prepared) { frame.blit(map_image.image, 0, 0); frame.image.colours = map_image.image.colours; }
  };
}
void mission_frontend::visible(std::string_view const name, bool const show) {
  auto &object{interface.named(name)}; object.flags = (object.flags & ~1u) | static_cast<unsigned int>(show);
}
void mission_frontend::list() {
  listed.clear(); selected = -1;
  for(size_t i{0}; i != definitions.size(); ++i) if(definitions[i].flow == flow && definitions[i].zone == zone) listed.push_back(i);
  std::ranges::sort(listed, [this](auto a, auto b){ return definitions[a].id < definitions[b].id; });
  std::string names, numbers, ticks;
  for(auto index : listed) {
    auto const &mission{definitions[index]};
    if(!names.empty()) { names += '\n'; numbers += '\n'; ticks += '\n'; }
    names += mission.title; numbers += std::to_string(mission.id);
    auto const found{std::ranges::find(pilot.missions, static_cast<uint32_t>(mission.id), &simulation::mission_history::mission)};
    unsigned int const state{found == pilot.missions.end() ? 0u : static_cast<unsigned int>(found->result)};
    ticks += state == 0 ? ' ' : state == 1 ? '-' : static_cast<char>(5 - state);
  }
  auto &entries{interface.named("_mission_selection_list")}; entries.text = names; entries.selected_line = -1; entries.start_line = 0;
  interface.named("_mission_selection_number_list").text = numbers;
  interface.named("_mission_selection_tick_list").text = ticks;
  interface.named("_mission_select_screen").picture = "menuscrn/" + zone_name(zone) + ".lbm";
  interface.named("_mission_select_background").picture = "menuscrn/" + zone_name(zone) + "_p.lbm";
  visible("_mission_select_background", false);
  visible("_training_infinite_weapons_button", flow == 2); visible("_training_no_crashes_button", flow == 2);
  for(auto &object : interface.objects()) if(object.action == "notify_mission_selection_level_") object.flags &= ~1u;
  interface.show("_mission_select_screen");
}
void mission_frontend::select(int const row) {
  selected = row == selected || row < 0 || static_cast<size_t>(row) >= listed.size() ? -1 : row;
  interface.named("_mission_selection_list").selected_line = selected;
  visible("_mission_select_background", selected >= 0); visible("_mission_selection_load_button", selected >= 0);
  if(selected >= 0) { interface.named("_mission_selection_info_text").text = text(definitions[listed[static_cast<size_t>(selected)]], "Description"); interface.named("_mission_selection_info_text").start_line = 0; }
}
void mission_frontend::prepare(int const id) {
  result.reset(); prepared = std::make_unique<mission_session>(data, id, options);
  auto const &definition{prepared->briefing}; flow = definition.flow; zone = definition.zone;
  auto const mid{static_cast<uint32_t>(id)};
  if(std::ranges::find(pilot.missions, mid, &simulation::mission_history::mission) == pilot.missions.end()) pilot.missions.push_back({mid});
  if(flow == 4 && std::ranges::find(pilot.campaign.missions, mid, &simulation::mission_history::mission) == pilot.campaign.missions.end()) pilot.campaign.missions.push_back({mid});
  int const parent{static_cast<int>(&interface.named("_map_screen") - interface.objects().data())};
  for(auto &object : interface.objects()) if(object.parent == parent) object.flags &= ~1u;
  for(auto const name : {"_map_briefing_button", "_map_takeoff_button", "_map_briefing_subtitle_box", "_map_exit_button"}) visible(name, true);
  interface.named("_map_briefing_subtitle_title").text = definition.title;
  interface.named("_map_briefing_subtitle_text").text = text(definition, "Description");
  interface.named("_map_briefing_subtitle_text").start_line = 0;
  subtitles(true); map(); interface.show("_map_screen");
}
void mission_frontend::subtitles(bool const expanded) {
  int const height{expanded ? 210 : 50};
  interface.bounds("_map_briefing_subtitle_box", {70, 478 - height, 570, 478});
  auto &body{interface.named("_map_briefing_subtitle_text")}; body.area.bottom = body.area.top + height - 20;
  auto &bar{interface.named("_map_briefing_subtitle_scrollbar")}; bar.area.bottom = bar.area.top + height - 20;
}
void mission_frontend::map() {
  auto &runtime{prepared->runtime}; auto const &actor{runtime.actors.units[*runtime.timing.player]};
  auto const &path{runtime.mission.tasks[actor.location.task].actors[actor.location.group].waypoints};
  int32_t left{actor.object.world.position[0]}, right{left}, bottom{actor.object.world.position[1]}, top{bottom};
  for(auto const &point : path) { left = std::min(left, point.position[0]); right = std::max(right, point.position[0]); bottom = std::min(bottom, point.position[1]); top = std::max(top, point.position[1]); }
  int32_t zoom{std::clamp(std::max((right - left) / 640 * 150 / 100, (top - bottom) / 480 * 150 / 100), 256, 0x33333)};
  int32_t cx{std::clamp((right + left) / 2, 0x2000000 + zoom * 320, 0xe000000 - zoom * 320)};
  int32_t cy{std::clamp((top + bottom) / 2, 0x2000000 + zoom * 240, 0xe000000 - zoom * 240)};
  render::window2 const window{cx - zoom * 320, cy - zoom * 240, cx + zoom * 320, cy + zoom * 240};
  map_image.reset_clip();
  auto const &palette{prepared->world.palette.colours};
  for(size_t i{0}; i != 256; ++i) map_image.image.colours[i] = {palette[i * 3], palette[i * 3 + 1], palette[i * 3 + 2]};
  prepared->world.draw_map(map_image, map_image.clip, window, 0, {}, runtime.scene.extra);
  render::projection2 view{map_image.clip, window}; render::rasterizer raster{map_image};
  for(size_t i{1}; i < path.size(); ++i) if(auto line{view.clip_line({path[i - 1].position[0], path[i - 1].position[1]}, {path[i].position[0], path[i].position[1]})}) raster.line((*line)[0], (*line)[1], 255);
}
void mission_frontend::campaign(bool const fresh) {
  if(fresh || pilot.campaign.warzone != static_cast<uint32_t>(zone) || !pilot.campaign.level) {
    pilot.campaign = {static_cast<uint32_t>(zone), 1, 4, static_cast<uint32_t>(std::chrono::system_clock::now().time_since_epoch().count()) & 0x7fff, {}}; last_result = 0;
  }
  campaign_mission = fresh ? 0 : pilot.next_mission(definitions);
  int level{static_cast<int>(pilot.campaign.level)}; std::string section{"Situation"};
  if(fresh) { level = 0; section = "Description"; }
  else if(campaign_mission == -2 || campaign_mission == -3) { level = 0; section = campaign_mission == -2 ? "Failure" : "Success"; }
  else if(last_result == 2) { ++level; section = "Failure"; }
  auto const raw{data.read("MISSDATA/C" + std::to_string(zone) + std::to_string(level) + ".TXT")};
  std::string_view const source{reinterpret_cast<char const*>(raw.data()), raw.size()};
  auto start{source.find(":" + section)};
  if(start == std::string_view::npos) throw std::runtime_error{"Missing campaign situation text"};
  start = source.find('\n', start);
  if(start == std::string_view::npos) throw std::runtime_error{"Truncated campaign situation text"};
  start = source.find_first_not_of("\r\n", start);
  auto const end{source.find(':', start)};
  auto &body{interface.named("_situation_text")}; body.text = source.substr(start, end - start);
  std::erase(body.text, '\r'); body.start_line = 0;
  interface.named("_situation_screen").picture = "menuscrn/c_" + zone_name(zone) + ".lbm";
  visible("_situation_ok_button", campaign_mission >= 0);
  if(campaign_mission == -3) { pilot.victories[static_cast<size_t>(zone - 1)] = true; pilot.campaign = {}; }
  else if(campaign_mission == -2) pilot.campaign = {};
  interface.show("_situation_screen");
}
std::unique_ptr<mission_session> mission_frontend::takeoff() {
  if(!departing) return {};
  departing = false; return std::move(prepared);
}
void mission_frontend::complete(std::unique_ptr<mission_session> mission) {
  prepared = std::move(mission);
  result = simulation::evaluate_debrief(prepared->runtime, prepared->records, prepared->runtime.random.global_seed, pilot.name);
  interface.named("_map_briefing_subtitle_title").text = result->title();
  auto &body{interface.named("_map_briefing_subtitle_text")}; body.text.clear(); body.start_line = 0;
  for(auto const &line : result->explanations) { if(!body.text.empty()) body.text += "\n\n"; body.text += line; }
  visible("_map_takeoff_button", false); visible("_map_exit_button", false); visible("_map_debrief_exit_button", true);
  visible("_map_debrief_log_do_not_log_box", false); map(); interface.show("_map_screen");
}
void mission_frontend::log_result(bool const keep) {
  if(!prepared || !result) return;
  if(keep) {
    result->log(pilot, static_cast<uint32_t>(prepared->briefing.id), static_cast<unsigned int>(flow), hours_now());
    pilot.update(definitions); last_result = static_cast<unsigned int>(result->mission); if(save) save(pilot);
  }
  prepared.reset(); result.reset(); interface.back();
  if(flow == 4) campaign(); else list();
}
void mission_frontend::back() {
  if(interface.current() == "_map_screen" && result) {
    visible("_map_debrief_log_do_not_log_box", true); visible("_map_debrief_retry_mission_button", true); visible("_map_debrief_do_not_log_mission_button", flow != 4);
  } else { if(interface.current() == "_map_screen") prepared.reset(); interface.back(); }
}
bool mission_frontend::activate(widget &object, int const line) {
  auto const &action{object.action};
  if(action == "notify_training_button_") { flow = 2; zone = 4; list(); }
  else if(action == "notify_single_button_" || action == "notify_campaign_button_") { flow = action == "notify_single_button_" ? 3 : 4; interface.show("_world_screen"); }
  else if(action == "notify_map_yemen_button_" || action == "notify_map_korea_button_" || action == "notify_map_cyprus_button_") {
    zone = action == "notify_map_yemen_button_" ? 1 : action == "notify_map_korea_button_" ? 2 : 3;
    if(flow == 4) campaign(pilot.campaign.warzone != static_cast<uint32_t>(zone)); else list();
  } else if(action == "notify_mission_selection_list_") select(line);
  else if(action == "notify_mission_selection_load_") { if(selected >= 0) prepare(definitions[listed[static_cast<size_t>(selected)]].id); }
  else if(action == "notify_situation_ok_button_") { if(campaign_mission == 0) campaign(); else if(campaign_mission > 0) prepare(campaign_mission); }
  else if(action == "notify_takeoff_button_") departing = prepared != nullptr;
  else if(action == "notify_debrief_log_button_") log_result(true);
  else if(action == "notify_debrief_do_not_log_button_") log_result(false);
  else if(action == "notify_debrief_retry_mission_button_") { if(prepared) prepare(prepared->briefing.id); }
  else if(action == "notify_mission_select_exit_button_" || action == "notify_map_exit_button_" || action == "notify_debrief_exit_button_" || action == "notify_situation_exit_button_") back();
  else if(action == "notify_briefing_subtitle_min_max_button_") { auto const &area{interface.named("_map_briefing_subtitle_box").area}; subtitles(area.bottom - area.top < 51); }
  else if(action == "notify_briefing_button_") visible("_map_briefing_subtitle_box", !(interface.named("_map_briefing_subtitle_box").flags & 1));
  else return false;
  return true;
}
} // namespace apache::frontend
