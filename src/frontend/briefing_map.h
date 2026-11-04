#pragma once
#include "map_layers.h"
#include "map_routes.h"
#include "menu.h"
#include "mission_session.h"
#include "configuration/settings.h"

namespace apache::frontend {
class briefing_map {
  menu &interface;
  configuration::settings &settings;
  mission_session *mission{nullptr};
  render::instrument_font font;
  render::framebuffer image{640, 480}, overlay{640, 480};
  std::vector<map_route_command> commands;
  bool dirty{true};
  int active_tool{0}, dragging{0}, start_x{0}, start_y{0}, half_width{20}, half_height{15};
  uint32_t start_angle{0};
  render::vertex2 cursor_world;
  void visible(std::string_view name, bool show);
  void show(std::string_view name);
  void fit();
  void refresh();
  void payload();
  void weather();
  uint32_t cursor_angle() const;
public:
  map_view view;
  briefing_map(resources::data_store const &data, menu &interface, configuration::settings &settings);
  void open(mission_session &session);
  void close() { mission = nullptr; dragging = 0; }
  void paint(render::framebuffer &target, widget const &object);
  void finish(render::framebuffer &target);
  bool activate(widget &object);
  bool pointer(widget &object, pointer_action action, int x, int y);
};
} // namespace apache::frontend
