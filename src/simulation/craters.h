#pragma once
#include <array>
#include <cstddef>
#include "math/geometry.h"
#include "resources/terrain.h"

namespace apache::simulation {
struct crater {
  math::vector3 position{0, 0, 0};
  int32_t model{-1};
  uint32_t heading{0};
  int32_t age{0};
  math::vector3 scale{0, 0, 0};
};
class craters {
  size_t next_short{0};
  size_t next_long{0};
public:
  std::array<crater, 128> short_lived;
  std::array<crater, 128> long_lived;
  void reset();
  bool add(math::vector3 const &position, int32_t model, uint32_t heading, bool temporary, resources::terrain const &terrain);
  bool add_random(math::vector3 const &position, int32_t model, bool temporary, resources::terrain const &terrain, uint32_t &seed);
  bool add_scalable(math::vector3 const &position, int32_t radius, bool heave, resources::terrain const &terrain, uint32_t &seed);
  void advance();
};
} // namespace apache::simulation
