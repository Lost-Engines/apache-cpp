#pragma once
#include <array>
#include <cstdint>

namespace apache::simulation {
struct actor_type {
  uint32_t flags{0}, classification{0}, aircraft{4}, ship{6};
  std::array<int32_t, 5> weapons{}, ammunition{};
  uint32_t main_weapon{0};
};
std::array<actor_type, 1315> actor_types();
} // namespace apache::simulation
