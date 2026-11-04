#pragma once
#include "math/geometry.h"
#include "projection.h"
#include "resources/features.h"
#include "scene_palette.h"
#include "terrain_shading.h"

namespace apache::render {
math::vector3 feature_origin(math::pose const &camera, int x_sector, int y_sector, bool super);
std::vector<vertex3> project_feature(resources::feature const &object, math::matrix3 const &rotation, math::vector3 const &origin, projection const &view);
class feature_renderer {
  framebuffer &target;
  projection const &view;
  scene_palette const &palette;
  rasterizer raster;
public:
  feature_renderer(framebuffer &destination, projection const &viewport, scene_palette const &colours);
  void draw(std::span<resources::feature_face const> faces, std::span<vertex3 const> points, terrain_fog const &fog);
};
} // namespace apache::render
