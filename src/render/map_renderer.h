#pragma once
#include "projection2.h"

namespace apache::render {
struct map_configuration {
  int32_t contour_interval{1000};
  uint32_t flags{6};
};
void draw_map_contours(framebuffer &target, projection2 const &view, std::array<vertex2, 3> triangle, int32_t interval);
} // namespace apache::render
