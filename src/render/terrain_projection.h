#pragma once
#include <vector>
#include "math/geometry.h"
#include "projection.h"
#include "resources/terrain.h"

namespace apache::render {
// Coordinates are returned in the terrain file's X-major order.
std::vector<vertex3> project_terrain(resources::terrain const &terrain, math::pose const &camera, projection const &view, int left, int bottom, unsigned int columns, unsigned int rows);
} // namespace apache::render
