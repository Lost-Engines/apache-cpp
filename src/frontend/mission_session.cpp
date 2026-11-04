#include "mission_session.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <stdexcept>
#include "resources/reader.h"

namespace apache::frontend {
namespace {
simulation::mission_random_state weapon_seeds() {
  simulation::mission_random_state result;
  auto const pair{[&](){ auto const high{(result.global = result.global * 0x41c64e6du + 0x3039u) & 0x7fff0000u}; result.global = result.global * 0x41c64e6du + 0x3039u; return high | ((result.global >> 16) & 0x7fff); }};
  result.damage = pair(); result.weapons = pair();
  return result;
}
resources::mission_briefing find_mission(resources::data_store const &data, int const id) {
  auto const missions{resources::read_mission_briefings(data.read("MISSDATA/MISSIONS.BLK"))};
  auto const found{std::ranges::find(missions, id, &resources::mission_briefing::id)};
  if(found == missions.end()) throw std::runtime_error{"Unknown mission " + std::to_string(id)};
  return *found;
}
std::string warzone(int const id) {
  static std::array<std::string, 4> const names{"YEMEN", "KOREA", "CYPRUS", "FORT"};
  if(id < 1 || id > 4) throw std::runtime_error{"Invalid mission warzone"};
  return names[static_cast<size_t>(id - 1)];
}
}
unsigned int mission_session::next_random() { library_random = library_random * 0x41c64e6du + 0x3039u; return (library_random >> 16) & 0x7fff; }
mission_session::mission_session(resources::data_store const &data, int const mission_id, mission_options const options)
  : settings{options}, initial_seeds{weapon_seeds()}, briefing{find_mission(data, mission_id)}, zone{warzone(briefing.zone)},
    runtime{data, zone, "MISSDATA/" + std::to_string(mission_id) + ".DTA", true, true, options.difficulty, initial_seeds},
    daylight{runtime.executable, runtime.mission.time_of_day}, world{data, zone}, panels{data, options.video_mode},
    avionics{panels, runtime.trig, runtime.executable}, frame{options.video_mode ? 640u : 320u, options.video_mode ? 480u : 240u} {
  auto const player{std::ranges::find_if(runtime.actors.units, [this](auto const &actor){ return runtime.mission.tasks[actor.location.task].user && actor.location.group == 0 && actor.location.member == 0; })};
  if(player == runtime.actors.units.end()) throw std::runtime_error{"Mission has no player aircraft"};
  runtime.timing.game_flow = briefing.flow;
  runtime.start_player(static_cast<size_t>(player - runtime.actors.units.begin()), {.arcade{options.arcade}, .flow{briefing.flow}});
  runtime.equip_player(runtime.mission.weapons); runtime.enable_player_sensors();
  runtime.player_damage->state.random = initial_seeds.damage;
  runtime.sensors->state.tads = false;
  controls.configuration.resolve();
  views = std::make_unique<simulation::player_views>(runtime.executable, runtime.trig, runtime.environment.height, runtime.sensors->state);
  runtime.player_armament->helmet_direction = [this](){
    if(views->modes.mode == 12) return runtime.player_armament->helmet;
    auto const &rotation{runtime.pilot->readings().position.rotation};
    return math::vector3{rotation[3], rotation[4], rotation[5]};
  };
  instruments = std::make_unique<simulation::mission_instruments>(runtime, options.video_mode);
  instruments->state.damaged_display_seed = 0;
  world.initialise_model_metadata(runtime.executable);
  world.bind_scene(runtime.scene.base, runtime.scene.extra, runtime.mobiles, runtime.weapons->visuals, runtime.damage->marks, runtime.debris);
  panels.install_palette(world.palette.colours);
  night_vision = daylight.night_vision(); instruments->state.helmet.colour = daylight.light.night == 0 ? 208 : 223;
  int const cloud_modifier{static_cast<int>(next_random() / 11)};
  visual.sky.weather = static_cast<unsigned int>(runtime.mission.weather);
  if(runtime.mission.weather == 1) { visual.cloud_height = cloud_modifier + 14000; visual.cloud_bottom = (cloud_modifier + 13000) * 256; visual.cloud_top = (cloud_modifier + 15000) * 256; }
  runtime.weapons->environment.weather = runtime.mission.weather;
  runtime.weapons->environment.cloud_bottom = visual.cloud_bottom; runtime.weapons->environment.cloud_top = visual.cloud_top;
  detail(options.detail);
  views->context.has_target = [this](size_t i){ auto *r{round(i)}; return r && simulation::projectile_has_target(*r, simulation::weapon_definitions()[r->state.weapon], runtime.targets.world, [this](uint32_t id, auto slot){ return runtime.weapons->target_slot(id, slot); }, runtime.timing.now); };
  views->context.has_lock = [this](size_t i){ auto *r{round(i)}; return r && simulation::weapon_definitions()[r->state.weapon].movement_class == 2 && (r->state.body.seeker.flags & 2); };
  views->crashed = [this](bool airborne){ crash(airborne); };
  runtime.pilot->died = [this](){ select_view(19); };
  runtime.events.projectile = [this](size_t i, auto &, auto const &event){ if(event.kind == simulation::projectile_event::removed) views->removed(i); };
  runtime.events.explosion_sound = [this](simulation::explosion_audio const &sound){
    // The original sound selector shares Watcom rand() with wind gusts, even
    // when no audio device is active. Keep that ordering in the live session.
    auto const sample{next_random() % 3 + 1};
    if(play_sound) play_sound(sample, sound.volume, sound.pan);
  };
  runtime.events.missile_camera = [this](size_t i){ views->context.latest_weapon = i; };
  runtime.events.flight = [this](simulation::flight_step_result const &result){
    if(result.landing) records.add({{12, 0, 0, 0, 0, result.landing->grade}});
    using enum simulation::flight_event;
    for(auto event : result.events) switch(event) {
      case fuel_exhausted: communications.add("OUT OF FUEL", 3, 2000); break;
      case hover_on: communications.add("AUTO HOVER ON", 3, 600); break;
      case hover_off: communications.add("AUTO HOVER OFF", 3, 600); break;
      case engine_overheat: communications.add("ENGINE OVERHEAT", 3, 2000); break;
      case left_engine_failed: communications.add("LHS ENGINE FAILED", 3, 2000); break;
      case right_engine_failed: communications.add("RHS ENGINE FAILED", 3, 2000); break;
      case engines_overheated: communications.add("ENGINE FAILED", 3, 2000); break;
      case overtorque_on: communications.add_changing(4, "ENGINE OVERTORQUE", 3); break;
      case overtorque_off: communications.remove(4); break;
      case overspeed_on: communications.add_changing(5, "VMAX EXCEEDED", 3); break;
      case overspeed_off: communications.remove(5); break;
      case pull_up_on: communications.add_changing(6, "PULL UP PULL UP", 3); break;
      case pull_up_off: communications.remove(6); break;
      default: break;
    }
  };
  runtime.events.landing = [this](auto const &landing){ if(landing.grade) records.add({{12, 0, 0, 0, 0, landing.grade}}); };
  runtime.events.killed = [this](auto const &target, auto const &source){
    uint32_t const who{source.kind == simulation::target_kind::mobile ? source.index : UINT32_MAX};
    if(target.kind == simulation::target_kind::mobile) records.add({{8, static_cast<uint32_t>(runtime.timing.now), who, 0, 0, target.index}});
    else if(target.kind == simulation::target_kind::scenery) records.add({{9, static_cast<uint32_t>(runtime.timing.now), who, static_cast<uint32_t>(target.layer), 0, target.index}});
  };
  runtime.events.notice = [this](auto text, auto priority, auto lifetime){ communications.add(text, priority, lifetime); };
  runtime.events.warning_message = [this](size_t slot, bool added, auto text){
    auto const id{static_cast<uint32_t>(slot + 1)};
    if(added) communications.add_changing(id, text, 2); else communications.remove(id);
  };
  select_view(views->modes.home);
}
void mission_session::detail(unsigned int const level) {
  if(level > 9) throw std::runtime_error{"Visual detail must be between 0 and 9"};
  settings.detail = level;
  visual.distance = level < 7 ? 1 : (level - 6) * 2 + 1;
  visual.far_object_depth = level == 0 ? 0x200000 : 0x400000;
  visual.terrain_detail = level < 2 ? 0 : level < 5 ? 1 : level == 5 ? 2 : 3;
  visual.models.textures = level >= 3; visual.models.gouraud = level >= 6;
  visual.sky.cloud_flags = level == 0 ? 0 : level < 4 ? 2 : 10;
  runtime.smoke_radius = 256; runtime.smoke_lifetime = level < 5 ? 400 : 600; runtime.smoke_density = level < 2 ? 1 : level < 6 ? 2 : 3;
}
simulation::projectile_record *mission_session::round(size_t const index) {
  if(index >= runtime.weapons->rounds.entries().size() || !runtime.weapons->rounds.entries()[index]) return nullptr;
  return &runtime.weapons->rounds.at(index);
}
std::optional<simulation::view_target> mission_session::missile_target(size_t const index) {
  auto *r{round(index)}; if(!r) return {};
  auto *target{simulation::adjusted_projectile_target(*r, runtime.targets.world, [this](uint32_t id, auto slot){ return runtime.weapons->target_slot(id, slot); })};
  if(!target || target->identity.kind == simulation::target_kind::none) return {};
  auto const description{runtime.targets.describe(*target)};
  return simulation::view_target{description.movement.position, runtime.models.at(description.model) ? runtime.models.at(description.model)->height : 0};
}
simulation::view_sources mission_session::sources() {
  auto const subject{[this](render::model_instance const &object){ return simulation::view_subject{object.world, object.id, runtime.models.at(object.id)->radius}; }};
  simulation::view_sources result; result.player = subject(runtime.actors.units[*runtime.timing.player].object); result.faults = runtime.pilot->faults();
  if(drone && *drone < runtime.actors.units.size()) result.drone = subject(runtime.actors.units[*drone].object);
  auto const &target{runtime.sensors->state.current};
  if(target.identity.kind != simulation::target_kind::none || runtime.sensors->state.locked) result.target = {runtime.targets.world.position(target), 0};
  result.weapon = [this, subject](size_t i) -> std::optional<simulation::view_subject> { auto *r{round(i)}; return r && r->mobile && runtime.mobiles.contains(*r->mobile) ? std::optional{subject(runtime.mobiles.at(*r->mobile))} : std::nullopt; };
  result.weapon_target = [this](size_t i){ return missile_target(i); };
  return result;
}
void mission_session::select_view(unsigned int const mode) {
  views->select(mode, sources(), runtime.random.global_seed);
  runtime.player_armament->in_cockpit = views->modes.in_cockpit;
}
void mission_session::crash(bool const airborne) {
  runtime.flight_active = false;
  runtime.spatial.unplace(*runtime.timing.player);
  auto &object{runtime.actors.units[*runtime.timing.player].object};
  render::fracture_model(object, runtime.debris, runtime.random.global_seed);
  if(airborne) {
    resources::reader bytes{runtime.executable.at(0x461510, 12), "crash velocity"};
    runtime.weapons->visuals.add_air_explosion(object.world.position, 0x3200, {bytes.i32(), bytes.i32(), bytes.i32()});
  } else runtime.weapons->visuals.add_ground_explosion(object.world.position, 0x3200, runtime.terrain);
}
void mission_session::key(uint16_t const scan, bool const down, bool const dispatch) {
  controls.key(scan & 127, down); if(down && dispatch) post(scan);
}
void mission_session::command(uint16_t const key) {
  auto &arm{*runtime.player_armament}; auto &sensors{runtime.sensors->state}; auto &displays{instruments->state.displays};
  auto const mode{arm.control_mode}; auto const faults{runtime.pilot->faults()}; bool const gunner{views->modes.crew == 1};
  simulation::targeting_events const target_events{.key_event{}, .cockpit_sound{runtime.events.sound ? std::function<void(unsigned int)>{[this](unsigned int id){ runtime.events.sound(id, true); }} : nullptr}, .show_radar{[&](){ displays.show_radar(gunner, faults); }}};
  if((key & 255) == 0x1c) { arm.select(runtime.timing.now, key); return; }
  if((key & 255) == 0x13) { sensors.cycle_radar(key, mode, target_events); return; }
  if((key & 255) == 0x26) { sensors.toggle_lock(key, mode, faults, target_events); return; }
  if(key >= 2 && key <= 11) {
    if(settings.arcade) { static std::array<double, 10> constexpr values{-.8, -.6, -.4, -.2, 0, .2, .4, .6, .8, 1}; runtime.pilot->arcade.delta_vertical_velocity = values[key - 2]; }
    else { runtime.pilot->realistic.dynamics.hover.active = false; runtime.pilot->realistic.dynamics.power.collective = static_cast<double>(key - 1) * 10.0; }
    return;
  }
  switch(key) {
    case 0x2001: case 0x2010: case 0x102d: finished = true; break;
    case 1: case 0x200f: accelerated_time = 1; break;
    case 0x400f: accelerated_time = 8; break;
    case 0xf: accelerated_time = accelerated_time == 8 ? 1 : accelerated_time * 2; break;
    case 0x19: paused = !paused; break;
    case 0x31: case 0x4031: {
      auto &actor{runtime.actors.units[*runtime.timing.player]};
      runtime.route.cycle(runtime.mission.tasks[actor.location.task].actors[actor.location.group].waypoints, actor.waypoint, key == 0x31, key, mode); break;
    }
    case 0x33: case 0x34:
      if(views->modes.in_cockpit) {
        if(views->modes.mode == 13) views->optics.zoom(13, key == 0x34);
        else instruments->state.map_range = key == 0x34 ? std::max(instruments->state.map_range / 2, 0x80000) : std::min(instruments->state.map_range * 2, 0x1000000);
      } break;
    case 0x17: instruments->state.helmet.colour = instruments->state.helmet.colour == 208 ? 223 : 208; break;
    case 0x2f: night_vision = !night_vision; break;
    case 0xe: case 0x400e: case 0xd: sensors.cycle_target(key == 0xe, mode); break;
    case 0x15: sensors.toggle_automatic(mode, faults); break;
    case 0x22: sensors.cycle_ground_range(key, mode, target_events); break;
    case 0x16: sensors.toggle_freeze(key, mode, target_events); break;
    case 0x25: sensors.toggle_tactical_range(); break;
    case 0x12: sensors.toggle_ecm(key, mode, target_events); break;
    case 0x32: case 0x4032:
      if(!sensors.air) sensors.threat = key == 0x32 ? (sensors.threat == 1 ? 4 : sensors.threat - 1) : (sensors.threat == 4 ? 1 : sensors.threat + 1);
      else if(sensors.air) sensors.scan_all = !sensors.scan_all;
      break;
    case 0x3c: case 0x47: select_view(0); break;
    case 0x3d: case 0x49: select_view(5); break;
    case 0x3e: case 0x4f: select_view(views->modes.left_view()); break;
    case 0x3f: case 0x51: select_view(views->modes.right_view()); break;
    case 0x40: select_view(10); break;
    case 0x23: select_view(12); break;
    case 0x14: views->select_optics(sources(), runtime.random.global_seed); break;
    case 0x41: select_view(14); break;
    case 0x42: select_view(15); break;
    case 0x43: select_view(18); break;
    case 0x44: select_view(17); break;
    case 0x57: select_view(16); break;
    case 0x58: case 0x4058: {
      if(runtime.actors.units.empty()) break;
      size_t candidate{drone.value_or(*runtime.timing.player)};
      for(size_t i{0}; i != runtime.actors.units.size(); ++i) {
        candidate = (candidate + (key == 0x58 ? 1 : runtime.actors.units.size() - 1)) % runtime.actors.units.size();
        auto const &actor{runtime.actors.units[candidate]};
        if(candidate != *runtime.timing.player && !actor.destroyed && runtime.mobiles.contains(runtime.spatial.mobile_id(candidate))) { drone = candidate; select_view(11); break; }
      } break;
    }
    case 0x39: case 0x2e: case 0x21: runtime.fire_player(runtime.timing.now, frame_time, key); break;
    case 0x202e: arm.inventory.automatic_chaff = !arm.inventory.automatic_chaff; break;
    case 0x1a: case 0x1b: case 0x401a: case 0x401b: displays.cycle(gunner, (key & 255) == 0x1b, (key & 0x4000) == 0); break;
    case 0x201a: case 0x201b: displays.toggle(gunner, (key & 255) == 0x1b, faults); break;
    case 0x18: if(settings.arcade) runtime.pilot->arcade.toggle_hover(); else runtime.pilot->realistic.dynamics.hover.toggle(runtime.pilot->realistic.dynamics.u); break;
    default: break;
  }
}
void mission_session::step(unsigned int const ticks) {
  if(ticks == 0 || ticks > 200) throw std::runtime_error{"Mission frame interval must be 1 to 200 ticks"};
  runtime.frame = ++frame_number;
  frame_time = static_cast<double>(ticks) * 0.005;
  last_delta = static_cast<int32_t>(std::min(frame_time * 2147483648.0, static_cast<double>(INT32_MAX)));
  controls.timer((keyboard_remainder + ticks) / 2); keyboard_remainder = (keyboard_remainder + ticks) % 2;
  daylight.update(runtime.timing.now);
  for(auto key : commands) command(key);
  commands.clear();
  if(paused || finished) return;
  if(controls.held[0x39]) runtime.fire_player(runtime.timing.now, frame_time, 0x39);
  for(unsigned int i{0}; i != accelerated_time; ++i) {
    controls.update(views->modes.movement, views->modes.crew == 1);
    runtime.pilot->realistic.acceleration = static_cast<int32_t>(accelerated_time);
    runtime.weapons->night = daylight.light.night != 0;
    runtime.frame = frame_number;
    runtime.advance(math::add(runtime.timing.now, static_cast<int32_t>(ticks)), last_delta, controls.flight, [this](){ return next_random(); });
    records.add({{2, ticks, 0, 0, 0, 0}});
    records.position(runtime.pilot->readings().position.position);
    communications.advance(frame_time);
    if(runtime.pilot->death() && views->modes.mode != 19) select_view(19);
  }
  runtime.player_armament->update_hydra(runtime.timing.now, last_delta, controls.keyboard[4]);
}
void mission_session::draw() {
  camera = views->update(sources(), {controls.external[3], controls.external[2], controls.keyboard[5], paused ? 0 : last_delta}, runtime.random.global_seed);
  finished = finished || camera.finished;
  visual.lighting = daylight.light; visual.infrared = night_vision || camera.infrared ? 1 : 0;
  visual.hidden_mobile = views->modes.in_cockpit ? std::optional{runtime.spatial.mobile_id(*runtime.timing.player)} : std::nullopt;
  auto const faders{panels.faders(daylight.light.night, night_vision)};
  world.palette.additional_faders.assign(faders.begin(), faders.end());
  world.palette.additional_faders.push_back({true, 10000, 2, 254, night_vision ? 1 : 0, {0, 1, 1}});
  world.palette.additional_faders.push_back({false, INT32_MAX, 0, 256, camera.fade, {0, 0, 0}});
  auto const viewport{views->modes.viewport(settings.video_mode)};
  frame.clip = {viewport[0], viewport[1], viewport[0] + viewport[2] - 1, viewport[1] + viewport[3] - 1};
  render::projection const projection{viewport[0], viewport[1], viewport[2], viewport[3]};
  world.draw(frame, camera.camera, visual);
  if(camera.sight) runtime.look_player(camera.camera, projection);
  auto &arm{*runtime.player_armament};
  auto const &rotation{views->modes.mode == 12 ? camera.camera.rotation : runtime.pilot->readings().position.rotation};
  arm.helmet = {rotation[3], rotation[4], rotation[5]}; arm.in_cockpit = views->modes.in_cockpit;
  arm.evaluate_sight(runtime.timing.now);
  instruments->update(views->modes.mode, camera.camera, projection);
  auto &state{instruments->state}; state.accelerated_time = accelerated_time;
  state.helmet.viewing_device = views->optics.name(); state.helmet.magnification = views->optics.magnification().label;
  auto const look{views->modes.mode == 13 ? views->goggles : views->helmet}; state.helmet.sight_heading = look.heading; state.helmet.sight_pitch = look.pitch;
  auto const &power{settings.arcade ? runtime.pilot->arcade.power : runtime.pilot->realistic.dynamics.power};
  auto const endurance{static_cast<int32_t>(std::clamp(power.endurance, 0.0, static_cast<double>(INT32_MAX)))};
  for(size_t i{0}; i != arm.warnings.entries.size(); ++i) communications.update_changing(static_cast<uint32_t>(i + 1), arm.warnings.entries[i].text);
  if(views->modes.mode == 0 || views->modes.mode == 5) state.communications = communications.compose({
    .fuel{runtime.pilot->readings().fuel}, .endurance{endurance}, .flow{runtime.timing.game_flow},
    .score{0}, .infinite_fuel{settings.arcade ? runtime.pilot->arcade.infinite_fuel : runtime.pilot->realistic.infinite_fuel}, .faults{runtime.pilot->faults()},
    .weapon{simulation::weapon_definitions()[arm.inventory.current].name}});
  frame.reset_clip(); panels.draw(frame, views->modes.mode, views->modes.changed); avionics.draw(frame, state, world, visual.lighting);
  for(size_t i{0}; i != 256; ++i) frame.image.colours[i] = {world.palette.modified[i * 3], world.palette.modified[i * 3 + 1], world.palette.modified[i * 3 + 2]};
  views->modes.changed = false;
}
} // namespace apache::frontend
