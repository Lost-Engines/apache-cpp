#include "mission_actors.h"
#include <algorithm>
#include <stdexcept>

namespace apache::simulation {
namespace {
bool stationary(int32_t const task) { return task == 10 || task == 11 || task == 16 || task == 20; }
int32_t distance(math::vector3 const &a, math::vector3 const &b) {
  uint64_t sum{0};
  for(size_t i{0}; i != 2; ++i) { int64_t const d{math::subtract(a[i], b[i])}; sum += static_cast<uint64_t>(d * d); }
  return std::bit_cast<int32_t>(math::integer_sqrt(sum));
}
math::vector3 offset(math::vector3 const &position, math::vector3 const &delta, uint32_t const heading, math::trigonometry const &trig) {
  auto const s{trig.sine(heading)}, c{trig.cosine(heading)};
  return {math::add(position[0], math::add(math::multiply_q31(delta[0], c), math::multiply_q31(delta[1], s))),
    math::add(position[1], math::subtract(math::multiply_q31(delta[1], c), math::multiply_q31(delta[0], s))), math::add(position[2], delta[2])};
}
uint32_t random_pair(uint32_t &seed) {
  seed = seed * 0x41c64e6du + 0x3039u;
  uint32_t const high{seed & 0x7fff0000u};
  seed = seed * 0x41c64e6du + 0x3039u;
  return high | ((seed >> 16) & 0x7fff);
}
render::model_instance clone(render::model_instance const &source) {
  render::model_instance result;
  result.id = source.id; result.flags = source.flags; result.alternate_colours = source.alternate_colours;
  result.relative = source.relative; result.world = source.world; result.scale = source.scale; result.distance = source.distance;
  for(auto const &child : source.children) result.children.push_back(child ? std::make_unique<render::model_instance>(clone(*child)) : nullptr);
  return result;
}
}
void mission_actors::synchronize_models() {
  for(auto &unit : units) if(!(unit.conditions & 0x400)) {
    unit.object.relative = unit.object.world; render::maintain_model(unit.object);
  }
  for(auto &carrier : carriers) {
    auto &ship{units[carrier.actor].object};
    for(auto const &slot : carrier.slots) if(slot.occupant) {
      auto &unit{units[*slot.occupant]};
      auto &attached{ship.children[slot.attachment]};
      if(!attached || attached->id != unit.object.id) attached = std::make_unique<render::model_instance>(clone(unit.object));
      attached->flags = unit.object.flags; attached->relative = slot.relative; attached->scale = unit.object.scale;
      render::maintain_model(*attached, &ship.world);
      unit.object.relative = slot.relative; unit.object.world = attached->world;
    }
  }
}
void mission_actors::register_ship(size_t const index, resources::model_archive const &models) {
  if(carriers.size() == 3) return;
  auto const &unit{units[index]}; auto const &definition{models.at(unit.object.id).value()};
  carrier_state ship{index, 0, {}};
  for(size_t slot{0}; slot != unit.object.children.size() && ship.slots.size() < 9; ++slot) {
    if(unit.object.children[slot]) continue;
    auto const &attachment{definition.children[slot]};
    ship.slots.push_back({slot, {attachment.translation, attachment.rotation}, {}});
  }
  if(ship.slots.empty()) return;
  ship.available = ship.slots.size(); units[index].carrier = carriers.size(); carriers.push_back(std::move(ship));
}
bool mission_actors::register_aircraft(size_t const index) {
  for(size_t carrier{0}; carrier != carriers.size(); ++carrier) {
    auto &ship{carriers[carrier]}; if(!ship.available) continue;
    size_t const slot{--ship.available}; auto &unit{units[index]};
    ship.slots[slot].occupant = index; unit.carrier = carrier; unit.carrier_slot = slot;
    unit.conditions |= 0x400; unit.object.relative = ship.slots[slot].relative;
    return true;
  }
  return false;
}
mission_actors::mission_actors(resources::mission_definition const &mission, std::span<std::vector<resources::formation_member> const> const formations, resources::model_archive const &models, math::trigonometry const &trig, actor_environment const &environment, unsigned int const map, uint32_t const seed) : types{actor_types()}, random_seed{seed} {
  for(size_t task_index{0}; task_index != mission.tasks.size(); ++task_index) {
    auto const &task{mission.tasks[task_index]};
    for(size_t group_index{0}; group_index != task.actors.size(); ++group_index) {
      auto const &definition{task.actors[group_index]};
      if(definition.formation >= formations.size()) throw std::runtime_error{"Unavailable formation"};
      auto const &formation{formations[definition.formation]};
      if(definition.formation && formation.empty()) throw std::runtime_error{"Empty named formation"};
      actor_group group{task_index, group_index, definition, {}};
      size_t const count{definition.formation ? formation.size() : 1};
      for(size_t member{0}; member != count; ++member) {
        uint32_t const model{definition.formation ? formation[member].model : definition.model};
        auto const &type{types.at(model)};
        actor_state unit; unit.location = {task_index, group_index, member}; unit.classification = type.classification;
        std::array<uint32_t, 20> attachments{};
        if(model == 607) { attachments[0] = 614; attachments[2] = 620; }
        else if(model == 990) attachments[0] = 859;
        unit.object = render::construct_model(models, model, {}, attachments);
        unit.object.alternate_colours = task.side == 2 && map != 2 && (type.flags & 2);
        unit.conditions |= definition.conditions;
        int weapons{0};
        for(size_t slot{0}; slot != type.weapons.size(); ++slot) if(type.weapons[slot]) {
          unit.ammunition[slot] = type.ammunition[slot]; ++weapons;
          unit.weapon = type.weapons[slot]; unit.weapon_slot = static_cast<int32_t>(slot);
        }
        if(task.type == 18) { if(!unit.ammunition[1]) ++weapons; unit.ammunition[1] = 10; }
        if(weapons == 1) unit.conditions |= 0xc0;
        else if(weapons > 1) { unit.conditions |= 0x40; unit.weapon = 0; unit.weapon_slot = 0; }
        else { unit.weapon = 0; unit.weapon_slot = -1; }
        unit.conditions &= ~0x30000u;
        if(unit.ammunition[0] > 0 || unit.ammunition[1] > 0) unit.conditions |= 0x20000;
        if(unit.ammunition[0] > 0) unit.conditions |= 0x10000;
        for(size_t slot{2}; slot != 5 && type.weapons[slot]; ++slot) if(unit.ammunition[slot] > 0) { unit.conditions |= 0x10000; break; }
        group.units.push_back(units.size()); units.push_back(std::move(unit));
        if(type.classification == 7) register_ship(units.size() - 1, models);
      }
      groups.push_back(std::move(group));
    }
  }
  for(auto &group : groups) {
    auto const &task{mission.tasks[group.task]}; auto const &definition{group.definition};
    auto const &route{definition.waypoints}; size_t const start{definition.start_waypoint};
    if(start >= route.size() || route.size() - start < 2) throw std::runtime_error{"Actor needs two starting waypoints"};
    auto &leader{units[group.units.front()]}; auto const &formation{formations[definition.formation]};
    leader.waypoint = start;
    bool on_carrier{false};
    if(leader.classification == 1) {
      if(route[start].position[2] == 0 && environment.surface(route[start].position[0], route[start].position[1]) == 3) on_carrier = register_aircraft(group.units.front());
      if(!on_carrier && route.back().position[2] == 0 && environment.surface(route.back().position[0], route.back().position[1]) == 3 && register_aircraft(group.units.front())) {
        leader.conditions &= ~0x400u;
        carriers[*leader.carrier].slots[*leader.carrier_slot].occupant.reset();
      }
    }
    if(!on_carrier) leader.angles[0] = trig.arctangent(math::subtract(route[start + 1].position[0], route[start].position[0]), math::subtract(route[start + 1].position[1], route[start].position[1]));
    leader.object.world.position = group.units.size() > 1 ? offset(route[start].position, formation[0].offset, leader.angles[0], trig) : route[start].position;
    auto &pose{leader.object.world};
    auto const ground{environment.height(pose.position[0], pose.position[1])};
    if((leader.classification != 1 && leader.classification != 2) || pose.position[2] <= ground) pose.position[2] = ground;
    leader.speed = stationary(task.type) ? 0 : route[start].speed;
    leader.waypoint = start + 1;
    leader.route_distance = distance(route[start + 1].position, pose.position);
    leader.velocity = {math::multiply_q31(trig.sine(leader.angles[0]), leader.speed), math::multiply_q31(trig.cosine(leader.angles[0]), leader.speed), 0};
    auto const s{trig.sine(leader.angles[0])}, c{trig.cosine(leader.angles[0])};
    pose.rotation = {c, math::negate(s), 0, s, c, 0, 0, 0, INT32_MAX};
    if(leader.classification == 1) {
      if(pose.position[2] && !on_carrier) leader.desired_speed = leader.speed;
      else leader.speed = 0;
    }
    if(task.type == 8 && route.front().type == 4) { leader.waypoint = 0; leader.conditions &= ~4u; }
    leader.random = random_pair(random_seed);
    // Simple members are linked in reverse order by create_simple_drone_.
    for(size_t member{group.units.size()}; member-- > 1;) {
      auto &unit{units[group.units[member]]}; auto const current{*leader.waypoint};
      if(!current) throw std::runtime_error{"Formation route has no previous waypoint"};
      auto const target{offset(route[current].position, formation[member].offset, leader.angles[0], trig)};
      unit.object.world.position = offset(route[current - 1].position, formation[member].offset, leader.angles[0], trig);
      unit.angles[0] = trig.arctangent(math::subtract(target[0], unit.object.world.position[0]), math::subtract(target[1], unit.object.world.position[1]));
      unit.velocity = {math::multiply_q31(trig.sine(unit.angles[0]), leader.speed), math::multiply_q31(trig.cosine(unit.angles[0]), leader.speed), 0};
      unit.object.world.rotation = trig.transformation(unit.angles[0], 0, 0);
      if(unit.classification == 5) unit.waypoint = 1;
      unit.random = random_pair(random_seed);
    }
    if(group.units.size() > 1) {
      size_t const current{*leader.waypoint}; auto const surface{environment.surface(route[current].position[0], route[current].position[1])};
      bool const independent{surface == 4 || surface == 5 || surface == 1 || route[current].position[2] || (current + 1 < route.size() && route[current + 1].position[2]) || (current && route[current - 1].position[2]) || (current > 1 && route[current - 2].position[2])};
      if(independent) {
        leader.conditions |= 0x200;
        for(size_t member{1}; member != group.units.size(); ++member) {
          auto &unit{units[group.units[member]]}; unit.waypoint = current; unit.route_distance = distance(route[current].position, unit.object.world.position);
          unit.angles[0] = trig.arctangent(math::subtract(route[current].position[0], unit.object.world.position[0]), math::subtract(route[current].position[1], unit.object.world.position[1]));
        }
      }
    }
    for(auto const index : group.units) {
      auto &unit{units[index]}; unit.speed = 0;
      if(stationary(task.type)) unit.conditions |= 0x40000;
      if(!unit.carrier_slot) unit.object.relative = unit.object.world;
    }
  }
  if(!groups.empty() && !groups.back().definition.waypoints.empty()) selected_waypoint = std::array{groups.size() - 1, groups.back().definition.waypoints.size() - 1};
  for(auto const &ship : carriers) for(auto const &slot : ship.slots) if(slot.occupant) {
    auto &parent{units[ship.actor].object}; parent.children[slot.attachment] = std::make_unique<render::model_instance>(clone(units[*slot.occupant].object));
  }
}
} // namespace apache::simulation
