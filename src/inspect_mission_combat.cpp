#include "inspect_mission_combat.h"
#include "simulation/mission_combat.h"
#include "resources/features.h"
#include <istream>
#include <ostream>
#include <iomanip>
#include <stdexcept>

void inspect_mission_combat(std::istream &input, std::ostream &output, apache::resources::data_store const &data, std::string const &zone, std::string const &path) {
  using namespace apache;
  auto const bytes{data.read(path)}; auto const mission{resources::read_mission_definition(std::string{bytes.begin(), bytes.end()})};
  resources::terrain const terrain{data.read(zone + ".WZ/GRIDPOST.BIN")}; auto models{resources::read_models(data.read("3DDATA/3DOBJS.BIN"))}; resources::executable_data const executable{data}; resources::initialise_model_metadata(models, executable);
  unsigned int const map{zone == "YEMEN" ? 1u : zone == "KOREA" ? 2u : zone == "CYPRUS" ? 3u : zone == "FORT" ? 4u : 0u};
  simulation::mission_scenery scene{mission, resources::read_scenery(data.read(zone + ".WZ/GNDOBJS.BIN")), terrain, models, map};
  resources::feature_layer const features{data, zone, false}, super_features{data, zone, true}; math::trigonometry const trig{data}; auto const formations{resources::read_formations(executable)};
  simulation::actor_environment environment{.height{[&](int32_t x, int32_t y){ return terrain.elevation(x, y); }}, .surface{[&](int32_t x, int32_t y){ return resources::surface_type(features, super_features, x, y); }}};
  simulation::combat_random random; random.next(); simulation::mission_actors actors{mission, formations, models, trig, environment, map, random.global_seed}; random.global_seed = actors.random_seed;
  render::mobile_scene mobiles{models}; simulation::mission_spatial spatial{mission, scene, actors, mobiles, terrain};
  environment.relocate = [&](simulation::actor_state &actor, math::vector3 const &position){ spatial.move(actor, position); };
  simulation::scenery_collisions structures{models, trig, scene.base, scene.extra};
  simulation::navigation_environment navigation{.height{environment.height}, .tallest_object{structures.tallest_object}, .obstacle{[&](auto const &from, auto const &to) -> std::optional<simulation::navigation_obstacle> { auto hit{structures.line(from, to)}; if(!hit) return {}; return simulation::navigation_obstacle{hit->position, models.at(structures.at(hit->layer, hit->index).model)->height}; }}, .relocate{environment.relocate}};
  simulation::mission_targets targets{mission, scene, actors, spatial, random, terrain, structures, mobiles, models, trig};
  simulation::combat_random weapon_random{1, random.global_seed}; simulation::combat_tables tables{executable};
  auto const reference{[&](simulation::target_reference const *target){
    if(!target) { output << " 0 0 0 0"; return; }
    auto const &t{target->identity};
    if(t.kind == simulation::target_kind::fixed) { output << " -1"; for(auto v : target->position) output << ' ' << v; }
    else output << ' ' << (t.kind == simulation::target_kind::mobile ? 200001 + static_cast<int64_t>(spatial.actor_index(t.index).value()) : 1 + t.index + t.layer * 100000) << " 0 0 0";
  }};
  size_t frame{0}; simulation::mission_combat *active{nullptr}; simulation::mission_combat_callbacks events;
  events.visible = [&](auto const &source, auto const &target){ return simulation::line_reaches_target(targets.world.position(source), targets.world.position(target), source.identity, target.identity, terrain, structures, mobiles, models, trig); };
  events.fire = [&](size_t i){ output << "fire " << frame << ' ' << i; reference(active->armaments[i].target.get()); output << '\n'; };
  events.ground_fire = [&](auto const &source, auto const &target, unsigned int id){ output << "groundfire " << frame << ' ' << id; reference(&source); reference(&target); output << '\n'; return simulation::firing_status::blocked; };
  events.sound = [&](unsigned int sound, bool cockpit){ output << "sound " << frame << ' ' << sound << ' ' << cockpit << '\n'; };
  events.message = [&](std::string_view message){ output << "message " << frame << ' ' << std::quoted(std::string{message}) << '\n'; };
  simulation::mission_combat combat{mission, scene, actors, spatial, targets, terrain, trig, tables, formations, environment, random, weapon_random, 0, events}; active = &combat;
  targets.additional_state = [&](uint32_t id){ simulation::mobile_target_state state; if(auto i{spatial.actor_index(id)}) state.damage = combat.attention[*i].damage; return state; };
  simulation::movement_callbacks callbacks{.check_air_target{[&](size_t i){ combat.check_air_lock(i); }}, .target{[&](size_t i){ return combat.movement_aim(i); }}, .kill{[&](size_t i){ combat.kill(i); }}, .fire_at_waypoint{[&](size_t i){ combat.fire_at_waypoint(i); }}, .place{[&](size_t i){ spatial.place(i); }}, .unplace{[&](size_t i){ spatial.unplace(i); }}, .weapon{[&](size_t i){ return combat.weapon(i); }}};
  simulation::navigation_tables const performance{executable}; simulation::actor_movement movement{actors, mission, formations, models, trig, environment, navigation, performance, random, callbacks};
  actors.synchronize_models();
  auto const snapshot{[&](){
    output << "snapshot " << frame << ' ' << random.value << ' ' << weapon_random.value << ' ' << random.global_seed << ' ' << combat.warnings.ships_attacked << ' ' << combat.warnings.radar << ' ' << combat.warnings.bandit << ' ' << combat.bombing_message_time << '\n';
    for(size_t i{0}; i != actors.units.size(); ++i) {
      auto const &a{actors.units[i]}; auto const &arm{combat.armaments[i]}; auto const &attention{combat.attention[i]};
      output << "actor " << i << ' ' << a.serial << ' ' << (a.conditions & ~0x8000u) << ' ' << a.object.status;
      for(auto v : a.object.world.position) output << ' ' << v; for(auto v : a.object.world.rotation) output << ' ' << v; for(auto v : a.angles) output << ' ' << v; for(auto v : a.velocity) output << ' ' << v;
      output << ' ' << a.speed << ' ' << (a.waypoint ? static_cast<int64_t>(*a.waypoint) : -1) << ' ' << a.destroyed << ' ' << a.desired_speed << ' ' << a.speed_ramp_target << ' ' << a.retreat_until << ' ' << a.weapon << ' ' << a.weapon_slot;
      for(auto v : a.ammunition) output << ' ' << v; for(auto v : arm.deadlines) output << ' ' << v;
      output << ' ' << arm.ground_maximum << ' ' << arm.ground_minimum << ' ' << arm.air_maximum << ' ' << arm.air_minimum << ' ' << attention.range << ' ' << static_cast<unsigned int>(attention.damage) << ' ' << static_cast<unsigned int>(attention.processed_damage) << ' ' << a.random;
      reference(arm.target.get()); output << '\n';
    }
    output << "alive"; for(auto v : combat.alive) output << ' ' << v; output << '\n';
    for(size_t i{0}; i != scene.objects.size(); ++i) { auto const &g{scene.objects[i].combat}; output << "defence " << i << ' ' << g.heading << ' ' << g.deadline << ' ' << g.shots; reference(g.target.get()); output << '\n'; }
    for(size_t i{0}; i != combat.battles.size(); ++i) { output << "battle " << i << ' ' << combat.battles[i].active; for(auto const &side : combat.battles[i].sides) output << ' ' << (side.task ? static_cast<int64_t>(*side.task) : -1) << ' ' << side.initial << ' ' << side.release_time; output << '\n'; }
    output << "patrol " << combat.patrols.last_scan << ' ' << combat.patrols.visible[0].size() << ' ' << combat.patrols.visible[1].size(); for(auto const &list : combat.patrols.visible) for(auto const &t : list) reference(t.get()); for(auto const &list : combat.patrols.assigned) for(auto const &t : list) reference(t.get()); output << '\n';
  }};
  simulation::movement_time timing; std::string command;
  while(input >> command) {
    if(command == "mode") input >> timing.arcade >> timing.game_flow;
    else if(command == "view") { for(auto &v : combat.player_position) input >> v; input >> combat.player_heading; }
    else if(command == "kill") { size_t i{0}; input >> i; combat.kill(i); }
    else if(command == "damage") { size_t i{0}; unsigned int amount{0}; input >> i >> amount; combat.attention.at(i).damage = static_cast<uint8_t>(amount); }
    else if(command == "snapshot") snapshot();
    else if(command == "step") { bool capture{false}; input >> timing.now >> timing.delta >> capture; combat.set_time(timing.now, timing.delta, static_cast<uint32_t>(frame)); movement.update(timing); combat.update(); actors.synchronize_models(); if(capture) snapshot(); ++frame; }
    else throw std::runtime_error{"Unknown mission combat command"};
    if(!input) throw std::runtime_error{"Incomplete mission combat command"};
  }
}
