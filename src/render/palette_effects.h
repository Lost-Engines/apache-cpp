#pragma once
#include <array>
#include <cstdint>
#include <span>

namespace apache::render {
using palette_rgb = std::array<uint8_t, 768>;
struct palette_fader {
  bool monochrome{false};
  int priority{0};
  unsigned int first{0};
  unsigned int count{256};
  int32_t level{0};
  std::array<uint8_t, 3> colour{0, 0, 0};
};
uint8_t luminance(uint8_t red, uint8_t green, uint8_t blue);
palette_rgb apply_faders(palette_rgb colours, std::span<palette_fader const> faders);
struct palette_conditions {
  unsigned int cloud_mode{0};
  unsigned int cloud_flags{0};
  unsigned int weather{0};
  int32_t altitude{0};
  int32_t cloud_bottom{6400000};
  int32_t cloud_top{6912000};
  int32_t night{0};
  unsigned int infrared{0};
  int32_t sun_glare{0};
  int32_t cockpit_glass{0};
};
} // namespace apache::render
