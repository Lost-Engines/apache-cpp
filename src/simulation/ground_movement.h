#pragma once
#include "mission_actors.h"

namespace apache::simulation {
// Route movement for ground vehicles and trains. Combat changes their flags and
// routes separately; the same movement path then advances or retreats the unit.
class ground_movement {
  std::vector<actor_state> &units;
  actor_group const &group;
  resources::mission_task const &task;
  std::span<resources::formation_member const> formation;
  math::trigonometry const &trig;
  actor_environment const &environment;
  int32_t delta{0};
  std::span<size_t const> members(actor_state const &unit) const;
  math::vector3 formation_point(math::vector3 position, size_t member, uint32_t heading) const;
  math::vector3 drive_to(actor_state const &leader, size_t member) const;
  void starting_speed(actor_state &unit);
  void zero_speed(actor_state &unit, bool stopped);
  void check_type(actor_state &unit);
  void past_waypoint(actor_state &unit, int32_t movement);
  void members_past_waypoint(actor_state &leader, int32_t movement, int32_t previous_distance);
  void move(actor_state &unit, actor_state const &leader, bool with_members);
public:
  ground_movement(std::vector<actor_state> &actors, actor_group const &actor_group, resources::mission_task const &orders, std::span<resources::formation_member const> members, math::trigonometry const &angles, actor_environment const &world);
  void update(int32_t now, int32_t delta_time);
  void finish_battle(bool advance, int32_t delta_time);
};
} // namespace apache::simulation
