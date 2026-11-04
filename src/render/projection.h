#pragma once
#include <span>
#include <vector>
#include "math/fixed.h"
#include "rasterizer.h"

namespace apache::render {
struct vertex3 {
  math::vector3 eye{0, 0, 0};
  screen_vertex screen;
  uint8_t outcode{0};
};
uint8_t outcode(math::vector3 const &point);
int winding(std::span<vertex3 const> vertices, bool robust = false);
class projection {
  rectangle viewport;
  int width{0};
  int height{0};
  int32_t vertical_scale{0};
  void project(vertex3 &point) const;
public:
  projection(int x, int y, int w, int h);
  vertex3 prepare(math::vector3 eye, int32_t u = 0, int32_t v = 0, int32_t intensity = 0) const;
  std::vector<vertex3> clip(std::span<vertex3 const> vertices) const;
};
} // namespace apache::render
