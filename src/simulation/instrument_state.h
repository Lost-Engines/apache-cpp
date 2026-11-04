#pragma once
#include <optional>
#include "math/trigonometry.h"

namespace apache::simulation {
struct instrument_waypoint {
  int32_t x{0};
  int32_t y{0};
  uint8_t letter{0};
};
struct instrument_inputs {
  math::vector3 position{0, 0, 0};
  uint32_t heading{0};
  uint32_t pitch{0};
  uint32_t roll{0};
  int32_t mission_time{0};
  int32_t game_time{0};
  int32_t terrain_height{0};
  int32_t climb_rate{0};
  int32_t airspeed{0};
  int32_t sidespeed{0};
  int32_t torque{0};
  std::optional<instrument_waypoint> waypoint;
};
struct instrument_state {
  uint32_t heading{0};
  uint32_t pitch{0};
  uint32_t roll{0};
  int32_t barometric_altitude{0};
  int32_t clock_time{0};
  int32_t radar_altitude{0};
  int32_t climb_rate{0};
  uint32_t command_heading{0};
  uint8_t waypoint_letter{0};
  int32_t waypoint_distance{0};
  int32_t waypoint_time{0};
  int32_t airspeed{0};
  int32_t sidespeed{0};
  int32_t torque{0};
};
instrument_state instrument_values(instrument_inputs const &input, math::trigonometry const &trig);
} // namespace apache::simulation
