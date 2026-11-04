#pragma once
#include <array>
#include <cstddef>
#include "math/geometry.h"
#include "resources/terrain.h"

namespace apache::simulation {
struct smoke {
  bool active{false};
  math::vector3 position{0, 0, 0};
  int32_t radius{0};
  int32_t lifetime{1};
  int32_t age{0};
  uint32_t angle{0};
};
struct explosion {
  unsigned int kind{0};
  math::vector3 position{0, 0, 0};
  math::vector3 velocity{0, 0, 0};
  int32_t radius{0};
  int32_t frame{-1};
};
class effects {
  size_t next_smoke{0};
  size_t next_explosion{0};
public:
  std::array<smoke, 2048> smoke_puffs;
  std::array<explosion, 128> explosions;
  void reset();
  bool add_smoke(math::vector3 const &position, int32_t radius, int32_t lifetime, uint32_t &random_seed);
  void add_trail(math::vector3 const &start, math::vector3 const &end, int32_t radius, int32_t lifetime, unsigned int density, uint32_t &random_seed);
  bool add_ground_explosion(math::vector3 const &position, int32_t radius, resources::terrain const &terrain);
  bool add_air_explosion(math::vector3 const &position, int32_t radius, math::vector3 const &velocity = {0, 0, 0});
  void advance(int32_t delta_time, resources::terrain const &terrain);
};
} // namespace apache::simulation
