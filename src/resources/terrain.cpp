#include "terrain.h"
#include <algorithm>
#include <stdexcept>

namespace apache::resources {
terrain::terrain(std::span<uint8_t const> const data) {
  reader input{data, "terrain grid"};
  if(input.size() != 257 * 257 * 8) input.fail("expected 257 by 257 eight-byte posts");
  posts.reserve(257 * 257);
  while(input.remaining()) {
    terrain_post value;
    value.height = input.u16();
    value.light = input.u8();
    value.colours = {input.u8(), input.u8()};
    value.flags = input.u8();
    value.textures = {input.u8(), input.u8()};
    posts.push_back(value);
  }
}
terrain_post const &terrain::post(unsigned int const x, unsigned int const y) const {
  if(x > 256 || y > 256) throw std::out_of_range{"terrain post outside map"};
  return posts[x * 257 + y];
}
int32_t terrain::elevation(int32_t const x, int32_t const y) const {
  /// Preserve the original two-stage integer triangle interpolation
  if(x < 0 || y < 0 || x >= 256 * terrain_step || y >= 256 * terrain_step) return 0;
  auto const ix{static_cast<unsigned int>(x >> 20)}, iy{static_cast<unsigned int>(y >> 20)};
  int32_t fx{x & (terrain_step - 1)}, fy{y & (terrain_step - 1)};
  int32_t a{post(ix, iy).height * 256}, b{post(ix + 1, iy + 1).height * 256};
  int32_t c{post(ix + 1, iy).height * 256}, d{post(ix, iy + 1).height * 256};
  if(!(post(ix, iy).flags & 64)) {
    std::swap(a, c);
    std::swap(b, d);
    fx = terrain_step - fx;
  }
  if(fy > fx) {
    std::swap(fx, fy);
    c = d;
  }
  if(!fx) return a;
  int64_t const edge{c + int64_t{fy} * (b - c) / fx};
  return a + static_cast<int32_t>((int64_t{fx} * (edge - a)) >> 20);
}

std::vector<scenery_placement> read_scenery(std::span<uint8_t const> const data) {
  reader input{data, "ground objects"};
  std::array<uint32_t, 4096> offsets;
  for(auto &offset : offsets) offset = input.u32();
  std::vector<scenery_placement> result;
  for(size_t sector{0}; sector != offsets.size(); ++sector) {
    auto const offset{offsets[sector]};
    if(!offset) continue;
    if(offset < offsets.size() * 4) input.fail("sector points into its offset table");
    input.seek(offset);
    unsigned int const count{input.u16()};
    if(count > input.remaining() / 12) input.fail("sector records exceed file");
    for(unsigned int i{0}; i != count; ++i) {
      scenery_placement item;
      item.sector = static_cast<uint16_t>(sector);
      item.model = input.u16();
      item.x = static_cast<int32_t>((sector / 64) << 22) + input.u16() * 256;
      item.y = static_cast<int32_t>((sector % 64) << 22) + input.u16() * 256;
      item.heading = input.u16();
      item.animation = input.u16();
      item.status = input.u16();
      result.push_back(item);
    }
  }
  return result;
}
} // namespace apache::resources
