#pragma once
#include <span>
#include "math/fixed.h"

namespace apache::render {
uint32_t approximate_range(math::vector3 const &point);
struct terrain_fog {
  uint32_t boundary{0};
  uint32_t width{1};
  explicit terrain_fog(unsigned int detail);
  uint32_t constant_colour(uint32_t range, uint8_t colour, std::span<uint8_t const, 256> remap) const;
  uint32_t shaded_colours(uint32_t range, unsigned int kind, std::span<uint8_t const, 256> remap) const;
  unsigned int texture_bank(uint32_t range, unsigned int terrain_mode) const;
};
} // namespace apache::render
