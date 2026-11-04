#pragma once
#include <array>
#include <span>
#include <vector>
#include "rasterizer.h"

namespace apache::render {
struct vertex2 {
  int32_t x{0};
  int32_t y{0};
  int32_t intensity{0};
};
struct window2 {
  int32_t left{0};
  int32_t bottom{0};
  int32_t right{0};
  int32_t top{0};
};
class projection2 {
  rectangle viewport;
  window2 window;
  std::array<int32_t, 2> centre{0, 0};
  std::array<int32_t, 2> scale{0, 0};
  uint8_t outcode(vertex2 const &point) const;
public:
  std::array<int32_t, 2> origin{0, 0};
  int32_t cosine{INT32_MAX};
  int32_t sine{0};
  projection2(rectangle viewport, window2 window);
  std::vector<screen_vertex> clip(std::span<vertex2 const> vertices) const;
};
} // namespace apache::render
