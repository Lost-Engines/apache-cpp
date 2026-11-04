#include "briefing_map.h"
#include <algorithm>
#include <bit>
#include <cmath>

namespace apache::frontend {
namespace {
std::array<std::string_view, 17> constexpr key_names{"", "Key Grid Lines", "Key Waypoints", "Key Other Way", "Key Hill Contours", "Key Compass Rose", "Key Allied AAA", "Key Allied SAM", "Key Allied EWR", "Key Allied Tanks", "Key Enemy AAA", "Key Enemy SAM", "Key Enemy EWR", "Key Enemy Tanks", "Key Sides", "Key Roads", "Key Rails"};
std::string weapon_name(int const kind) {
  switch(kind) { case 2: return "Hellfire Laser"; case 3: return "Hellfire Radar"; case 4: return "Hydra 255"; case 5: return "Hydra 261"; case 6: return "Stingers"; default: return "None"; }
}
}
briefing_map::briefing_map(resources::data_store const &data, menu &ui, configuration::settings &preferences)
  : interface{ui}, settings{preferences}, font{resources::executable_data{data}, true} {}
void briefing_map::visible(std::string_view const name, bool const value) { auto &object{interface.named(name)}; object.flags = (object.flags & ~1u) | static_cast<unsigned int>(value); }
void briefing_map::show(std::string_view const name) { visible(name, true); interface.raise(name); }
void briefing_map::open(mission_session &session) {
  mission = &session; active_tool = dragging = 0;
  for(auto &object : interface.objects()) {
    if(object.action == "notify_map_tool_" && object.type == 10) object.on = object.item == 0;
    if(object.action == "notify_map_key_button_" && object.item > 0 && object.item < 17) object.on = settings.values.integer(key_names[static_cast<size_t>(object.item)]).value_or(1) != 0;
  }
  auto &r{mission->runtime}; auto const &actor{r.actors.units.at(*r.timing.player)};
  commands = map_route_commands(r.mission.tasks.at(actor.location.task), actor.location.group, settings.values.integer("Key Other Way").value_or(0) != 0);
  fit(); visible("_map_tool_box", true);
}
void briefing_map::fit() {
  std::vector<render::vertex2> points;
  for(auto const &command : commands) if(command.kind == 1 || command.kind == 2) points.push_back({command.x, command.y});
  view.fit(points); dirty = true;
}
void briefing_map::refresh() {
  auto const &r{mission->runtime}; auto const &palette{mission->world.palette.colours};
  image.reset_clip();
  for(size_t i{0}; i != 256; ++i) image.image.colours[i] = {palette[i * 3], palette[i * 3 + 1], palette[i * 3 + 2]};
  image.image.colours[245] = {255, 0, 0};
  mission->world.draw_map(image, image.clip, view.window(), view.position.rotation, {250, 31});
  auto const enabled{[&](unsigned int item, bool fallback = true){ return settings.values.integer(key_names[item]).value_or(fallback ? 1 : 0) != 0; }};
  overlay.clear();
  draw_map_overlay(overlay, view, r.trig, r.scene, r.mission.destroy_markers, {enabled(1), enabled(5), enabled(14), true, enabled(6), enabled(7), enabled(9, false), enabled(10), enabled(11), enabled(13, false)});
  image.blit(overlay.image, 0, 0, true); overlay.clear();
  draw_map_routes(overlay, view, commands, r.trig, font, enabled(2), enabled(3, false));
  image.blit(overlay.image, 0, 0, true); dirty = false;
}
void briefing_map::paint(render::framebuffer &target, widget const &object) {
  if(!mission || &object != &interface.named("_map_screen")) return;
  if(dirty) refresh();
  target.blit(image.image, 0, 0); target.image.colours = image.image.colours;
  if(dragging == 1) {
    render::rasterizer raster{target};
    int const l{start_x - half_width}, r{start_x + half_width}, t{start_y - half_height}, b{start_y + half_height};
    raster.line({l, t}, {r, t}, 245); raster.line({r, t}, {r, b}, 245); raster.line({r, b}, {l, b}, 245); raster.line({l, b}, {l, t}, 245);
  }
}
void briefing_map::finish(render::framebuffer &target) {
  if(!mission || interface.current() != "_map_screen") return;
  render::palette_rgb palette;
  for(size_t i{0}; i != 256; ++i) { auto const &c{target.image.colours[i]}; palette[i * 3] = c.r; palette[i * 3 + 1] = c.g; palette[i * 3 + 2] = c.b; }
  mission->world.palette.apply_terrain_palette(palette);
  for(size_t i{0}; i != 256; ++i) target.image.colours[i] = {palette[i * 3], palette[i * 3 + 1], palette[i * 3 + 2]};
}
void briefing_map::weather() {
  auto const &weather{mission->runtime.mission};
  interface.named("_map_met_strength_text").text = std::to_string(weather.wind_strength) + " Kts";
  int degrees{std::bit_cast<int32_t>(weather.wind_direction) / 0xb60b61}; if(degrees < 0) degrees += 360;
  interface.named("_map_met_direction_text").text = std::to_string(degrees) + " Degs";
  interface.named("_map_met_weather_condition_text").text = std::array<std::string, 4>{"Clear", "Overcast", "Light Fog", "Thick Fog"}.at(static_cast<size_t>(weather.weather));
}
void briefing_map::payload() {
  auto &r{mission->runtime}; auto const &weapons{r.mission.weapons};
  r.player_armament->rearm(weapons);
  interface.named("_payload_wingtip_button").text = weapon_name(weapons[4]);
  interface.named("_payload_pod1_button").text = weapon_name(weapons[0]);
  interface.named("_payload_pod2_button").text = weapon_name(weapons[2]);
  for(auto &object : interface.objects()) if(!object.globals.empty() && object.globals.front().starts_with("_payload_") && object.type == 16 && object.globals.front() != "_payload_graphic") object.flags &= ~1u;
  auto const suffix{[](int id){ return id == 2 ? "h" : id == 3 ? "hr" : id == 4 || id == 5 ? "hd" : ""; }};
  for(std::string const side : {"r", "l"}) {
    std::string outer{suffix(weapons[2])};
    if(weapons[4] == 6) visible("_payload_" + side + "w_st" + (outer.empty() ? "" : "_" + outer), true);
    else if(!outer.empty()) visible("_payload_" + side + "o_" + outer, true);
    std::string const inner{suffix(weapons[0])}; if(!inner.empty()) visible("_payload_" + side + "i_" + inner, true);
  }
}
bool briefing_map::activate(widget &object) {
  if(!mission || interface.current() != "_map_screen") return false;
  if(object.action == "notify_map_key_button_" && object.item > 0 && object.item < 17) {
    object.on = !object.on; settings.set(key_names[static_cast<size_t>(object.item)], object.on ? 1 : 0); dirty = true; return true;
  }
  if(object.action == "notify_payload_wingtip_button_" || object.action == "notify_payload_pod1_button_" || object.action == "notify_payload_pod2_button_") {
    size_t const slot{object.action == "notify_payload_wingtip_button_" ? 4u : object.action == "notify_payload_pod1_button_" ? 0u : 2u};
    std::vector<std::pair<std::string, int>> choices{{weapon_name(0), 0}};
    if(slot == 4) choices.emplace_back(weapon_name(6), 6);
    else for(int i{2}; i <= 5; ++i) choices.emplace_back(weapon_name(i), i);
    interface.popup(std::move(choices), [this, slot](int kind){ if(!mission) return; mission->runtime.mission.weapons[slot] = mission->runtime.mission.weapons[slot + 1] = kind; payload(); });
    return true;
  }
  if(object.action != "notify_map_tool_") return false;
  switch(object.item) {
    case 2: view.pop(); dirty = true; break;
    case 3: fit(); break;
    case 6: show("_map_key"); break;
    case 8: weather(); show("_map_met_box"); break;
    case 18: visible("_map_3d_view_box", false); payload(); show("_map_payload_box"); break;
    default:
      active_tool = object.item;
      for(auto &tool : interface.objects()) if(tool.action == "notify_map_tool_" && tool.type == 10) tool.on = tool.item == active_tool;
      visible("_map_waypoint_tool_box", false);
      break;
  }
  return true;
}
uint32_t briefing_map::cursor_angle() const {
  int32_t const x{cursor_world.x - view.position.x}, y{cursor_world.y - view.position.y};
  if(!x && !y) return 0;
  long double const turns{std::atan2(static_cast<long double>(x), static_cast<long double>(y)) * static_cast<long double>(0.3183098861837697) * 2147483647.0L};
  return 0u - static_cast<uint32_t>(static_cast<int64_t>(turns));
}
bool briefing_map::pointer(widget &object, pointer_action const action, int const x, int const y) {
  if(!mission || interface.current() != "_map_screen" || &object != &interface.named("_map_screen")) return false;
  cursor_world = view.projection(mission->runtime.trig).unproject(x, y);
  if(action == pointer_action::right_down) { view.centre(cursor_world.x, cursor_world.y); dirty = true; return true; }
  if(action == pointer_action::left_down) {
    start_x = x; start_y = y; half_width = 20; half_height = 15;
    dragging = active_tool == 1 || active_tool == 4 ? active_tool : 0; start_angle = cursor_angle();
  } else if(action == pointer_action::move && dragging == 1) {
    half_width = std::max(20, std::abs(x - start_x)); half_height = std::max(15, std::abs(y - start_y));
    if(half_height * 640 / 480 < half_width) half_height = half_width * 480 / 640;
    else half_width = half_height * 640 / 480;
  } else if(action == pointer_action::left_up) {
    if(dragging == 1) {
      auto const centre{view.projection(mission->runtime.trig).unproject(start_x, start_y)};
      view.push(); view.set(centre.x, centre.y, static_cast<int32_t>(int64_t{view.position.zoom} * half_width / 320), view.position.rotation); dirty = true;
    } else if(dragging == 4) { view.rotate(view.position.rotation + cursor_angle() - start_angle); dirty = true; }
    dragging = 0;
  }
  return true;
}
} // namespace apache::frontend
