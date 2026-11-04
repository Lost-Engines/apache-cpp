#pragma once
#include <array>
#include <string_view>
#include "resources/data_store.h"

namespace apache::render {
class scene_palette {
  void spread(unsigned int colour, unsigned int first, unsigned int count);
public:
  std::array<uint8_t, 768> colours{};
  std::array<uint8_t, 65536> intensity{};
  std::array<std::array<uint8_t, 65536>, 4> fog_intensity{};
  std::array<uint8_t, 256> glass{};
  std::array<uint8_t, 256> visual_remap{};
  scene_palette(resources::data_store const &data, std::string_view zone, bool overcast = false);
};
} // namespace apache::render
