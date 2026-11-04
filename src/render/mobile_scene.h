#pragma once
#include "draw_queue.h"
#include "shadows.h"

namespace apache::render {
struct mobile_visibility {
  unsigned int cloud_mode{0};
  unsigned int cloud_flags{0};
  unsigned int weather{0};
  int32_t cloud_bottom{6400000};
  int32_t cloud_top{6912000};
  int32_t near_depth{-262144};
  int32_t far_depth{4194304};
  bool shadows{false};
};
class mobile_scene {
  struct entry {
    model_instance object;
    std::optional<size_t> sector;
  };
  resources::model_archive const &models;
  std::vector<std::optional<entry>> objects;
  std::array<std::vector<uint32_t>, 4096> sectors;
  void unplace(uint32_t id);
public:
  explicit mobile_scene(resources::model_archive const &definitions) : models{definitions} {}
  uint32_t add(uint32_t model, math::pose const &pose);
  void move(uint32_t id, math::pose const &pose);
  void remove(uint32_t id);
  void clear();
  bool contains(uint32_t id) const { return id < objects.size() && objects[id].has_value(); }
  model_instance &at(uint32_t id);
  model_instance const &at(uint32_t id) const;
  std::span<uint32_t const> sector(size_t index) const { return sectors.at(index); }
  std::optional<int32_t> shadow_receiver(math::vector3 const &position) const;
  void submit(draw_queue &queue, math::pose const &camera, mobile_visibility const &visibility, shadow_projection const &casting) const;
};
} // namespace apache::render
