#pragma once
#include <cstdint>

namespace apache::multiplayer {
enum class game_mode : unsigned int { single_player, combat, single_missions, pilot_gunner, multiplayer, capture_the_flag, apache_vs_hind };
struct game_settings {
  game_mode mode{game_mode::combat};
  bool second_seat{false}, night{false}, difficult{true};
  int32_t weather{0};
  bool infinite_fuel{false}, infinite_weapons{false}, crashes{true};
};
struct mission_rules {
  int32_t control{2}, rank{1}, forces{9}, radio{100};
  int32_t wind_strength{0}, wind_direction{0}, weather{0}, time_of_day{8640000};
  bool client_server{true}, no_crashes{false}, infinite_fuel{false}, infinite_weapons{false};
};
mission_rules apply_rules(game_settings const &settings, mission_rules mission = {});
} // namespace apache::multiplayer
