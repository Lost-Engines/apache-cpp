#include "mission_network.h"

namespace apache::simulation {
std::optional<size_t> mission_network::actor(uint32_t const connection) const {
  if(connection >= state.players.size()) return {};
  for(auto const &group : runtime.actors.groups) {
    size_t const index{group.units.front()};
    if(runtime.actors.units[index].network == static_cast<int32_t>(connection)) return index;
  }
  return {};
}
bool mission_network::bind(uint32_t const connection, size_t const index) {
  if(connection >= state.players.size() || index >= runtime.actors.units.size() || runtime.actors.units[index].location.member) return false;
  if(auto previous{actor(connection)}; previous && *previous != index) return false;
  auto &unit{runtime.actors.units[index]};
  if(unit.network != -1 && unit.network != static_cast<int32_t>(connection)) return false;
  unit.network = static_cast<int32_t>(connection); state.players[connection].assigned = true;
  return true;
}
std::optional<target_reference> mission_network::resolve(multiplayer::target_state const &target) const {
  if(target.kind == 2) {
    for(size_t i{0}; i < runtime.actors.units.size(); ++i) {
      auto const &unit{runtime.actors.units[i]};
      if(unit.serial == target.data[0] && static_cast<int64_t>(unit.location.task) == target.data[1]) return *runtime.spatial.targets[i];
    }
  } else if(target.kind == 1) {
    auto const &position{target.data};
    if(position[0] < 0 || position[0] >= 0x10000000 || position[1] < 0 || position[1] >= 0x10000000) return {};
    // locate_ground_object uses the stored 256-unit coordinate cell, not the
    // object's collision bounds, altitude, visibility or damage flags.
    for(size_t layer{0}; layer < 2; ++layer) {
      auto const &objects{layer ? runtime.scene.extra : runtime.scene.base};
      for(size_t i{0}; i < objects.size(); ++i) {
        auto const &object{objects[i]};
        if(object.model && object.sector == (position[0] >> 22) * 64 + (position[1] >> 22) && (object.x >> 8) == (position[0] >> 8) && (object.y >> 8) == (position[1] >> 8)) return target_reference{{target_kind::scenery, layer, static_cast<uint32_t>(i)}, position, 0};
      }
    }
  }
  return {};
}
multiplayer::target_state mission_network::identify(target_reference const &target) const {
  auto const &id{target.identity};
  if(id.kind == target_kind::mobile) {
    if(auto index{runtime.spatial.actor_index(id.index)}) {
      auto const &unit{runtime.actors.units[*index]};
      return {2, {unit.serial, static_cast<int32_t>(unit.location.task), 0}};
    }
  } else if(id.kind == target_kind::scenery && runtime.structures.contains(id.layer, id.index)) {
    return {1, runtime.targets.world.position(target)};
  }
  return {};
}
bool mission_network::receive(multiplayer::packet const &message, int32_t const now) {
  if(!enabled || !state.receive(message, now)) return false;
  if(message.type() == multiplayer::message_type::position) {
    if(auto index{actor(message.sender())}) {
      auto const target{resolve(state.players[message.sender()].target)};
      auto &slot{runtime.combat.armaments[*index].target};
      if(target) { if(!slot) slot = std::make_shared<target_reference>(); *slot = *target; }
      else slot.reset();
    }
  }
  return true;
}
void mission_network::update() {
  if(!enabled) return;
  for(auto const &group : runtime.actors.groups) {
    size_t const index{group.units.front()}; auto &unit{runtime.actors.units[index]};
    if(runtime.spatial.player == index || unit.network < 0 || unit.network >= static_cast<int32_t>(state.players.size()) || unit.destroyed == 1 || (unit.conditions & 0x400)) continue;
    auto &remote{state.players[static_cast<size_t>(unit.network)]};
    unit.speed = std::bit_cast<int32_t>(static_cast<uint32_t>(remote.aircraft.speed) * 432u);
    unit.object.world.rotation = remote.aircraft.location.rotation;
    runtime.spatial.move(unit, remote.aircraft.location.position);
    auto const &position{remote.aircraft.location.position};
    if(position[0] && position[1] && position[2] && remote.weapon) {
      auto const target{remote.weapon == 1 || remote.weapon == 19 ? std::nullopt : resolve(remote.shot_target)};
      if(runtime.weapons) runtime.weapons->fire_remote(index, remote.weapon, remote.shot_seed, remote.shot_target.data, target);
      remote.weapon = 0;
    }
  }
}
} // namespace apache::simulation
