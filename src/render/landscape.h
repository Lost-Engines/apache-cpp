#pragma once
#include "draw_queue.h"
#include "feature_renderer.h"
#include "horizon.h"
#include "sector_scan.h"
#include "terrain_renderer.h"
#include "resources/features.h"

namespace apache::render {
struct landscape_options {
  unsigned int distance{7};
  unsigned int terrain_detail{3};
  bool features{true};
  sky_conditions sky{.ground_colour{166}};
};
class landscape {
  resources::terrain terrain;
  resources::terrain_textures textures;
  resources::feature_layer features;
  math::trigonometry trig;
public:
  scene_palette palette;
  landscape(resources::data_store const &data, std::string_view zone);
  resources::terrain const &heights() const { return terrain; }
  void draw(framebuffer &target, math::pose camera, landscape_options const &options);
};
} // namespace apache::render
