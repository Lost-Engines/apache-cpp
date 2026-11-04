#pragma once
#include <array>
#include <optional>
#include <string_view>
#include <vector>
#include "data_store.h"
#include "reader.h"

namespace apache::resources {
struct feature_vertex {
  uint8_t flags{0};
  std::array<std::array<int16_t, 2>, 4> components{};
};
struct feature_face {
  uint8_t type{0};
  uint8_t surface{0};
  std::array<uint8_t, 2> colours{0, 0};
  bytes indices;
  bytes intensities;
  bytes render_indices;
};
struct feature {
  bool has_distant{false};
  std::vector<feature_vertex> vertices;
  std::vector<feature_face> faces;
  std::vector<feature_face> distant;
};
class feature_layer {
public:
  bool super{false};
  std::vector<std::optional<feature>> records;
  std::vector<uint32_t> placements;
  feature_layer(data_store const &data, std::string_view zone, bool super);
};
} // namespace apache::resources
