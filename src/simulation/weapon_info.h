#pragma once
#include <array>
#include <cstdint>
#include <string_view>

namespace apache::simulation {
struct weapon_info {
  int32_t movement_class{0}, guidance{0};
  uint8_t flags{0}, alternate_penetration{0}, burst_size{0}, weapon_role{0}, damage{0}, decoy_denominator{0};
  int32_t motor_acceleration{0}, motor_ticks{0}, burst_pause_ticks{0}, guidance_start_distance{0}, blast_radius{0};
  int32_t minimum_pitch{0}, maximum_pitch{0}, heading_limit{0}, penetration{0};
  int32_t minimum_range{0}, maximum_range{0}, minimum_speed{0}, maximum_speed{0}, launch_speed{0};
  int32_t model{0}, target_box_half_size{0}, fire_interval_ticks{0}, lateral_acceleration{0}, tracking_or_dispersion{0}, unresolved_5c{0};
  double drag_coefficient{0};
  std::string_view name;
};
std::array<weapon_info, 55> weapon_definitions();
} // namespace apache::simulation
