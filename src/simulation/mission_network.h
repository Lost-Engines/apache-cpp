#pragma once
#include "mission_runtime.h"
#include "multiplayer/replication.h"
#include "multiplayer/rules.h"
#include "multiplayer/lifecycle.h"
#include "multiplayer/casualties.h"

namespace apache::simulation {
// Connects APNET state to the mission's existing render, collision and target objects.
class mission_network {
  mission_runtime &runtime;
  std::array<std::optional<size_t>, 16> assignments;
  void replace_model(size_t actor_index, uint32_t model, unsigned int map);
public:
  multiplayer::replication state;
  multiplayer::lifecycle lifecycle;
  multiplayer::casualties casualties;
  std::function<void(target_identity const &, uint8_t)> remote_killed;
  std::function<void(multiplayer::packet const &)> send;
  bool enabled{false};
  explicit mission_network(mission_runtime &mission);
  std::optional<size_t> actor(uint32_t connection) const;
  bool bind(uint32_t connection, size_t actor_index);
  bool assign(uint32_t connection, uint32_t model, size_t task, multiplayer::game_mode mode, int flow, unsigned int map);
  void remove_unassigned();
  bool revive(uint32_t connection, int32_t model);

  std::optional<target_reference> resolve(multiplayer::target_state const &target) const;
  multiplayer::target_state identify(target_reference const &target) const;
  bool receive(multiplayer::packet const &message, int32_t now);
  void update();
};
} // namespace apache::simulation
