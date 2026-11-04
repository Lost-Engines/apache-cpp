#include "terrain_textures.h"
#include <stdexcept>
#include <string>

namespace apache::resources {
terrain_textures::terrain_textures(data_store const &data, std::string_view const zone) {
  auto const load{[&](std::string_view const name, std::span<texture> const textures, unsigned int const width){
    auto const bytes{data.read(std::string{zone} + ".WZ/" + std::string{name})};
    reader input{bytes, name};
    if(bytes.size() != textures.size() * width * width) input.fail("Invalid terrain texture length");
    for(auto &image : textures) {
      image.width = width;
      auto const pixels{input.take(width * width)};
      image.pixels.assign(pixels.begin(), pixels.end());
    }
  }};
  load("TEXT64.RAW", detailed, 64);
  load("TEXT64SP.RAW", spots, 64);
  load("TERRTEXT.BMP", coarse, 32);
  triangle_colours = data.read("3DDATA/TERRTEXT.CMP");
  if(triangle_colours.size() != 97 * 256) throw std::runtime_error{"Invalid terrain triangle colour map"};
}
std::span<uint8_t const, 64> terrain_textures::subdivisions(bool const forward, unsigned int const half, unsigned int selector) const {
  if(half > 1 || selector > 127) throw std::runtime_error{"Invalid terrain subdivision selector"};
  if(selector >= 97) selector = 0;
  return std::span<uint8_t const, 64>{triangle_colours.data() + ((forward ? 0 : 194) + half * 97 + selector) * 64, 64};
}
texture const &terrain_textures::get(bool const forward, unsigned int selector, bool const high_detail) const {
  if(selector > 127) throw std::runtime_error{"Invalid terrain texture selector"};
  if(high_detail && selector >= 97) return spots[selector - 97 + (forward ? 0 : 31)];
  if(selector >= 97) selector = 0;
  return (high_detail ? detailed : coarse)[selector + (forward ? 0 : 97)];
}
} // namespace apache::resources
