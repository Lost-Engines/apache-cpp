#pragma once
#include "celestial.h"
#include "draw_queue.h"
#include "feature_renderer.h"
#include "horizon.h"
#include "model_renderer.h"
#include "sector_scan.h"
#include "terrain_renderer.h"
#include "resources/features.h"

namespace apache::render {
struct landscape_options {
  unsigned int distance{7};
  unsigned int terrain_detail{3};
  bool features{true};
  bool scenery{true};
  bool celestial_bodies{true};
  sky_lighting lighting;
  int32_t near_object_depth{-262144};
  int32_t far_object_depth{4194304};
  model_style models;
  sky_conditions sky{.ground_colour{166}};
};
class landscape {
  resources::terrain terrain;
  celestial sky_objects;
  resources::terrain_textures textures;
  resources::feature_layer features;
  math::trigonometry trig;
  resources::model_archive models;
  std::vector<std::optional<resources::texture>> object_textures;
  std::vector<model_instance> scenery;
  std::array<std::vector<size_t>, 4096> scenery_sectors;
public:
  scene_palette palette;
  int32_t sun_glare{0};
  size_t missing_normals{0};
  size_t invalid_texture_samples{0};
  landscape(resources::data_store const &data, std::string_view zone);
  resources::terrain const &heights() const { return terrain; }
  void draw(framebuffer &target, math::pose camera, landscape_options const &options);
};
} // namespace apache::render
