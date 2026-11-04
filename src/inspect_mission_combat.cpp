#include "inspect_mission_combat.h"
#include "simulation/mission_runtime.h"
#include <istream>
#include <ostream>
#include <iomanip>
#include <stdexcept>

void inspect_mission_combat(std::istream &input, std::ostream &output, apache::resources::data_store const &data, std::string const &zone, std::string const &path, bool const armed, bool const hurt) {
  using namespace apache;
  simulation::mission_runtime runtime{data, zone, path, armed, hurt};
  runtime.check_collisions = runtime.subdivide_flight = false;
  auto &mission{runtime.mission}; auto &scene{runtime.scene}; auto &terrain{runtime.terrain}; auto &models{runtime.models}; auto &trig{runtime.trig};
  auto &actors{runtime.actors}; auto &spatial{runtime.spatial}; auto &mobiles{runtime.mobiles}; auto &structures{runtime.structures}; auto &targets{runtime.targets};
  auto &random{runtime.random}; auto &weapon_random{runtime.weapon_random}; auto &combat{runtime.combat}; auto &pilot{runtime.pilot}; auto &player_damage{runtime.player_damage};
  auto &launched{runtime.weapons}; auto &damage{runtime.damage}; auto &check_collisions{runtime.check_collisions}; auto &collision_marks{runtime.collision_marks}; auto &timing{runtime.timing}; auto &frame{runtime.frame};
  auto const identity{[&](simulation::target_identity const &id) -> int64_t {
    if(id.kind == simulation::target_kind::scenery) return 1 + id.index + id.layer * 100000;
    if(id.kind == simulation::target_kind::mobile) {
      if(auto i{spatial.actor_index(id.index)}) return 200001 + static_cast<int64_t>(*i);
      if(launched) if(auto i{launched->projectile_index(id.index)}) return 300001 + static_cast<int64_t>(*i);
    }
    return 0;
  }};
  auto const reference{[&](simulation::target_reference const *target){
    if(!target || target->identity.kind == simulation::target_kind::none) { output << " 0 0 0 0"; return; }
    auto const &t{target->identity};
    if(t.kind == simulation::target_kind::fixed) { output << " -1"; for(auto v : target->position) output << ' ' << v; }
    else output << ' ' << identity(t) << " 0 0 0";
  }};
  auto &events{runtime.events};
  events.fire = [&](size_t i){ output << "fire " << frame << ' ' << i; reference(combat.armaments[i].target.get()); output << '\n'; };
  events.ground_fire = [&](auto const &source, auto const &target, unsigned int id){ output << "groundfire " << frame << ' ' << id; reference(&source); reference(&target); output << '\n'; };
  events.sound = [&](unsigned int sound, bool cockpit){ output << "sound " << frame << ' ' << sound << ' ' << cockpit << '\n'; };
  events.message = [&](std::string_view text){ output << "message " << frame << ' ' << std::quoted(std::string{text}) << '\n'; };
  events.launched = [&](size_t i, auto const &round){ output << "launch " << frame << ' ' << i << ' ' << round.state.weapon << '\n'; };
  events.projectile = [&](size_t i, auto &, auto const &info){ output << "event " << frame << ' ' << i << ' ' << static_cast<unsigned int>(info.kind); for(auto v : info.position) output << ' ' << v; output << ' ' << (info.target ? identity(*info.target) : 0) << '\n'; };
  events.killed = [&](auto const &target, auto const &source){ output << "kill " << frame << ' ' << identity(target) << ' ' << identity(source) << '\n'; };
  events.explosion_sound = [&](auto const &sound){ output << "explosion_sound " << frame << ' ' << sound.volume << ' ' << sound.pan << '\n'; };
  events.collision = [&](auto const &id){ output << "collisionkill " << frame << ' ' << identity(id) << '\n'; };
  events.impact_sound = [&](bool heavy){ if(heavy) output << "explosion_sound " << frame << " 63 32768\n"; else output << "metal_sound " << frame << '\n'; };
  events.radar_toggle = [&](unsigned int value){ output << "radar-toggle " << frame << ' ' << value << '\n'; };
  auto const snapshot{[&](){
    if(pilot) {
      if(check_collisions) output << "contact " << pilot->collision_model << ' ' << (pilot->hit_actor ? static_cast<int64_t>(*pilot->hit_actor) : -1) << '\n';
      auto const &readings{pilot->readings()}; output << "flight " << pilot->on_carrier() << ' ' << pilot->landed() << ' ' << pilot->death() << ' ' << (pilot->arcade.power.faults | pilot->realistic.dynamics.power.faults);
      for(auto v : readings.angles) output << ' ' << v;
      output << ' ' << readings.airspeed << ' ' << readings.sidespeed << ' ' << readings.climb << ' ' << readings.torque << ' ' << readings.fuel;
      for(auto v : readings.engines) output << ' ' << static_cast<int32_t>(v); output << '\n';
    }
    if(player_damage) { auto const &state{player_damage->state}; output << "player-damage " << pilot->faults() << ' ' << state.random << ' ' << state.broken_mfds << ' ' << state.warning_at << ' ' << identity(player_damage->killer) << '\n'; }
    output << "snapshot " << (frame ? frame - 1 : 0) << ' ' << random.value << ' ' << weapon_random.value << ' ' << random.global_seed << ' ' << combat.warnings.ships_attacked << ' ' << combat.warnings.radar << ' ' << combat.warnings.bandit << ' ' << combat.bombing_message_time << '\n';
    for(size_t i{0}; i != actors.units.size(); ++i) {
      auto const &a{actors.units[i]}; auto const &arm{combat.armaments[i]}; auto const &attention{combat.attention[i]};
      output << "actor " << i << ' ' << a.serial << ' ' << (a.conditions & ~0x8000u) << ' ' << a.object.status;
      for(auto v : a.object.world.position) output << ' ' << v; for(auto v : a.object.world.rotation) output << ' ' << v; for(auto v : a.angles) output << ' ' << v; for(auto v : a.velocity) output << ' ' << v;
      output << ' ' << a.speed << ' ' << (a.waypoint ? static_cast<int64_t>(*a.waypoint) : -1) << ' ' << a.destroyed << ' ' << a.desired_speed << ' ' << a.speed_ramp_target << ' ' << a.retreat_until << ' ' << a.weapon << ' ' << a.weapon_slot;
      for(auto v : a.ammunition) output << ' ' << v; for(auto v : arm.deadlines) output << ' ' << v;
      output << ' ' << arm.ground_maximum << ' ' << arm.ground_minimum << ' ' << arm.air_maximum << ' ' << arm.air_minimum << ' ' << attention.range << ' ' << static_cast<unsigned int>(attention.damage) << ' ' << static_cast<unsigned int>(attention.processed_damage) << ' ' << a.random;
      reference(arm.target.get()); output << '\n';
    }
    if(launched) {
      for(size_t i{0}; i != actors.units.size(); ++i) { auto const &d{launched->decoys[i]}; output << "decoys " << i << ' ' << d.deadlines[0] << ' ' << d.deadlines[1]; for(auto const &target : d.targets) reference(target.get()); output << ' ' << (combat.armaments[i].round ? static_cast<int64_t>(*combat.armaments[i].round) : -1) << '\n'; }
      output << "rounds " << launched->rounds.active_count() << '\n';
      auto const definitions{simulation::weapon_definitions()};
      for(size_t i{0}; i != launched->rounds.entries().size(); ++i) if(launched->rounds.entries()[i]) {
        auto &round{launched->rounds.at(i)}; auto const &state{round.state}; auto const &body{state.body}; auto const kind{definitions[state.weapon].movement_class};
        output << "round " << i << ' ' << state.weapon << ' ' << state.launched_at << ' ' << state.random << ' ' << body.motion.flags;
        if(kind == 3) { for(auto v : state.impact) output << ' ' << v; output << ' ' << state.impact_at; }
        else {
          output << ' ' << body.motion.distance << ' ' << body.motion.speed;
          for(auto v : body.pose.position) output << ' ' << v; for(auto v : body.pose.rotation) output << ' ' << v; for(auto v : body.previous) output << ' ' << v; for(auto v : body.angles) output << ' ' << v;
          output << ' ' << (kind == 2 ? body.seeker.flags : 0) << ' ' << (kind == 2 ? body.seeker.retry_time : 0);
          auto const &children{mobiles.at(*round.mobile).children}; output << ' ' << (children.empty() || !children[0] ? 0 : children[0]->id);
          reference(kind == 2 ? simulation::adjusted_projectile_target(round, targets.world, {}) : nullptr);
        }
        output << '\n';
      }
      resources::bytes smoke; size_t count{0};
      auto const word{[&](uint32_t value){ for(unsigned int j{0}; j != 4; ++j) smoke.push_back(static_cast<uint8_t>(value >> (j * 8))); }};
      for(size_t i{0}; i != launched->visuals.smoke_puffs.size(); ++i) {
        auto const &puff{launched->visuals.smoke_puffs[i]}; if(!puff.active) continue; ++count;
        word(static_cast<uint32_t>(i)); word(1); for(auto v : puff.position) word(static_cast<uint32_t>(v)); word(static_cast<uint32_t>(puff.radius)); word(static_cast<uint32_t>(puff.lifetime)); word(static_cast<uint32_t>(puff.age)); word(puff.angle);
      }
      output << "smoke " << count << ' ' << resources::fingerprint(smoke) << '\n';
    }
    if(damage) {
      resources::bytes raw; size_t count{0};
      auto const word{[&](uint32_t value){ for(unsigned int j{0}; j != 4; ++j) raw.push_back(static_cast<uint8_t>(value >> (j * 8))); }};
      auto const vector{[&](math::vector3 const &v){ for(auto x : v) word(static_cast<uint32_t>(x)); }};
      for(size_t i{0}; i != launched->visuals.explosions.size(); ++i) {
        auto const &e{launched->visuals.explosions[i]}; if(!e.kind) continue;
        ++count; word(static_cast<uint32_t>(i)); word(e.kind); vector(e.position); vector(e.velocity); word(static_cast<uint32_t>(e.radius)); word(static_cast<uint32_t>(e.frame));
      }
      for(size_t i{0}; i != 256; ++i) {
        auto const &c{i < 128 ? damage->marks.short_lived[i] : damage->marks.long_lived[i - 128]}; if(c.model < 0) continue;
        ++count; word(static_cast<uint32_t>(i + 128)); vector(c.position); word(static_cast<uint32_t>(c.model)); word(c.heading); word(static_cast<uint32_t>(c.age)); vector(c.scale);
      }
      output << "effects " << count << ' ' << resources::fingerprint(raw) << '\n';
      raw.clear();
      for(auto const &layer : {std::cref(scene.base), std::cref(scene.extra)}) for(auto const &object : layer.get()) { word(object.model); word(object.heading); word(object.status); }
      output << "scenery " << resources::fingerprint(raw) << '\n';
    }
    output << "alive"; for(auto v : combat.alive) output << ' ' << v; output << '\n';
    for(size_t i{0}; i != scene.objects.size(); ++i) { auto const &g{scene.objects[i].combat}; output << "defence " << i << ' ' << g.heading << ' ' << g.deadline << ' ' << g.shots; reference(g.target.get()); output << '\n'; }
    for(size_t i{0}; i != combat.battles.size(); ++i) { output << "battle " << i << ' ' << combat.battles[i].active; for(auto const &side : combat.battles[i].sides) output << ' ' << (side.task ? static_cast<int64_t>(*side.task) : -1) << ' ' << side.initial << ' ' << side.release_time; output << '\n'; }
    output << "patrol " << combat.patrols.last_scan << ' ' << combat.patrols.visible[0].size() << ' ' << combat.patrols.visible[1].size(); for(auto const &list : combat.patrols.visible) for(auto const &t : list) reference(t.get()); for(auto const &list : combat.patrols.assigned) for(auto const &t : list) reference(t.get()); output << '\n';
  }};
  simulation::flight_input controls; std::string command;
  while(input >> command) {
    if(command == "mode") input >> timing.arcade >> timing.game_flow;
    else if(command == "pilot") {
      size_t index{0}; simulation::mission_flight_settings settings; input >> index >> settings.arcade >> settings.fuel >> settings.flow >> settings.faults; for(auto &id : settings.pods) input >> id;
      runtime.start_player(index, settings);
    }
    else if(command == "player-damage-state") {
      if(!player_damage) throw std::runtime_error{"Player damage is not active"};
      auto &state{player_damage->state}; input >> state.random >> state.broken_mfds >> state.warning_at >> state.ground_radar >> state.air_radar;
    }
    else if(command == "player-hit") {
      if(!player_damage) throw std::runtime_error{"Player damage is not active"};
      unsigned int amount{0}; input >> amount; combat.attention[*timing.player].damage = static_cast<uint8_t>(amount); player_damage->apply(simulation::damage_action::player_systems, {}, targets.now);
    }
    else if(command == "subdivide") input >> runtime.subdivide_flight;
    else if(command == "collisions") input >> check_collisions;
    else if(command == "settle-carrier") {
      if(!pilot) throw std::runtime_error{"A carrier test requires a player"};
      pilot->advance(0.05, 107374182, {}, 0);
      output << "settled " << pilot->on_carrier() << ' ' << pilot->landed() << ' ' << pilot->death();
      for(auto v : pilot->readings().position.position) output << ' ' << v; for(auto v : pilot->readings().position.rotation) output << ' ' << v; for(auto v : pilot->readings().angles) output << ' ' << v; output << '\n';
    }
    else if(command == "contact-test") {
      if(!pilot) throw std::runtime_error{"A contact test requires a player"};
      math::pose pose; math::vector3 previous; std::array<uint32_t, 3> angles; double forward{0}, vertical{0}; bool landed{false}; unsigned int cause{0};
      for(auto &v : pose.position) input >> v; for(auto &v : pose.rotation) input >> v; for(auto &v : previous) input >> v; for(auto &v : angles) input >> v; input >> forward >> vertical >> landed >> cause;
      auto &actor{actors.units[*timing.player]}; actor.conditions &= ~0x400u;
      pilot->realistic.on_carrier = pilot->arcade.on_carrier = false; pilot->realistic.landed = pilot->arcade.landed = landed; pilot->realistic.death = pilot->arcade.death = cause;
      pilot->realistic.readings.position = pilot->arcade.readings.position = pose; pilot->realistic.readings.angles = pilot->arcade.readings.angles = angles;
      pilot->realistic.dynamics.u = pilot->arcade.forward_velocity = forward; pilot->realistic.dynamics.zdot = vertical; pilot->arcade.delta_vertical_velocity = vertical / 72;
      actor.object.world.rotation = pose.rotation; spatial.move(actor, pose.position); render::maintain_model(actor.object);
      pilot->collide(previous, structures, mobiles, damage ? damage->marks : collision_marks, [&](auto const &id){ if(damage) damage->destroy(id); else output << "collisionkill " << frame << ' ' << identity(id) << '\n'; });
      output << "response " << pilot->collision_model << ' ' << (pilot->hit_actor ? static_cast<int64_t>(*pilot->hit_actor) : -1) << ' ' << pilot->landed() << ' ' << pilot->death() << ' ' << pilot->on_carrier() << ' ' << (actor.carrier ? static_cast<int64_t>(*actor.carrier) : -1) << ' ' << (actor.carrier_slot ? static_cast<int64_t>(*actor.carrier_slot) : -1);
      for(auto v : pilot->readings().position.position) output << ' ' << v; for(auto v : pilot->readings().position.rotation) output << ' ' << v; for(auto v : pilot->readings().angles) output << ' ' << v;
      for(auto const &carrier : actors.carriers) for(auto const &slot : carrier.slots) output << ' ' << (slot.reserved ? static_cast<int64_t>(*slot.reserved) : -1);
      output << '\n';
    }
    else if(command == "controls") { for(auto &v : controls.controls) input >> v; input >> controls.absolute_throttle >> controls.type; }
    else if(command == "view") { for(auto &v : combat.player_position) input >> v; input >> combat.player_heading; }
    else if(command == "kill") { size_t i{0}; input >> i; combat.kill(i); }
    else if(command == "damage") { size_t i{0}; unsigned int amount{0}; input >> i >> amount; combat.attention.at(i).damage = static_cast<uint8_t>(amount); }
    else if(command == "snapshot") snapshot();
    else if(command == "step") {
      bool capture{false}; int32_t now{0}, delta{0}; input >> now >> delta >> capture;
      runtime.advance(now, delta, controls, [](){ return 0; });
      if(capture) snapshot();
    }
    else throw std::runtime_error{"Unknown mission combat command"};
    if(!input) throw std::runtime_error{"Incomplete mission combat command"};
  }
}
