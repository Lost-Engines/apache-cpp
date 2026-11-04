#pragma once
#include "mission_runtime.h"
#include "multiplayer/replication.h"

namespace apache::simulation {
// Connects APNET state to the mission's existing render, collision and target objects.
class mission_network {
  mission_runtime &runtime;
public:
  multiplayer::replication state;
  bool enabled{false};
  explicit mission_network(mission_runtime &mission) : runtime{mission} {}
  std::optional<size_t> actor(uint32_t connection) const;
  bool bind(uint32_t connection, size_t actor_index);
  std::optional<target_reference> resolve(multiplayer::target_state const &target) const;
  multiplayer::target_state identify(target_reference const &target) const;
  bool receive(multiplayer::packet const &message, int32_t now);
  void update();
};
} // namespace apache::simulation
