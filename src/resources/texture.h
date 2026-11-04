#pragma once
#include <optional>
#include <vector>
#include "reader.h"

namespace apache::resources {
struct texture {
  unsigned int width{0};
  bytes pixels;
};
std::vector<std::optional<texture>> read_object_textures(std::span<uint8_t const> data);
} // namespace apache::resources
