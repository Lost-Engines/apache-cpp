#pragma once
#include <array>
#include <string_view>
#include "data_store.h"
#include "texture.h"

namespace apache::resources {
class terrain_textures {
  std::array<texture, 194> detailed;
  std::array<texture, 194> coarse;
  std::array<texture, 62> spots;
  bytes triangle_colours;
public:
  terrain_textures(data_store const &data, std::string_view zone);
  std::span<uint8_t const, 64> subdivisions(bool forward, unsigned int half, unsigned int selector) const;
  texture const &get(bool forward, unsigned int selector, bool high_detail) const;
};
} // namespace apache::resources
