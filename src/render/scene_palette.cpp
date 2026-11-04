#include "scene_palette.h"
#include <algorithm>
#include <stdexcept>
#include <string>

namespace apache::render {
void scene_palette::spread(unsigned int const colour, unsigned int const first, unsigned int const count) {
  for(unsigned int level{0}; level != 256; ++level) intensity[colour * 256 + level] = static_cast<uint8_t>(first + level * count / 256);
}
scene_palette::scene_palette(resources::data_store const &data, std::string_view const zone, bool const overcast) {
  auto const load{[&](std::string const &name, size_t const first, size_t const count){
    auto const file{data.read(name)};
    if(file.size() < count * 3) throw std::runtime_error{"Truncated palette: " + name};
    std::copy_n(file.begin(), count * 3, colours.begin() + static_cast<ptrdiff_t>(first * 3));
  }};
  for(unsigned int colour{0}; colour != 256; ++colour) {
    spread(colour, colour, 0);
    visual_remap[colour] = static_cast<uint8_t>(colour);
  }
  load("3DDATA/3DPAL.RGB", 0, 224);
  colours[203 * 3] = 64; colours[203 * 3 + 1] = 64; colours[203 * 3 + 2] = 128;
  std::array<std::array<unsigned int, 2>, 14> constexpr ramps{{{2, 3}, {5, 12}, {17, 12}, {29, 12}, {41, 8}, {49, 8}, {57, 7}, {64, 7}, {156, 12}, {71, 12}, {83, 16}, {99, 16}, {115, 12}, {127, 8}}};
  for(auto const &[first, count] : ramps) for(unsigned int i{0}; i != count; ++i) spread(first + i, first, i + 1);
  spread(155, 155, 1);
  spread(0xaa, 0x57, 0);
  spread(0xab, 0x56, 1);
  spread(0xac, 0x56, 2);
  spread(0xad, 0x55, 3);
  spread(0xae, 0x55, 4);
  spread(0xaf, 0x54, 5);
  spread(0xb0, 0x54, 6);
  spread(0xb1, 0x53, 7);
  spread(0xb4, 0x57, 0);
  spread(0xb5, 0x56, 1);
  spread(0xb6, 0x56, 1);
  spread(0xb7, 0x55, 2);
  spread(0xb8, 0x55, 2);
  spread(0xb9, 0x54, 2);
  spread(0xba, 0x54, 2);
  spread(0xbb, 0x53, 3);
  spread(0xbe, 0x67, 0);
  spread(0xbf, 0x66, 1);
  spread(0xc0, 0x66, 2);
  spread(0xc1, 0x65, 3);
  spread(0xc2, 0x65, 4);
  spread(0xc3, 100, 5);
  spread(0xc4, 100, 6);
  spread(0xc5, 99, 7);
  spread(200, 0x67, 0);
  spread(0xc9, 0x66, 2);
  spread(0xca, 0x66, 4);
  spread(0xcb, 0x65, 6);
  spread(0xcc, 0x65, 8);
  spread(0xcd, 100, 10);
  spread(0xce, 100, 0xc);
  spread(0xcf, 99, 0xe);
  spread(0xd2, 0x7a, 0);
  spread(0xd3, 0x79, 1);
  spread(0xd4, 0x79, 1);
  spread(0xd5, 0x78, 3);
  spread(0xd6, 0x78, 3);
  spread(0xd7, 0x77, 4);
  spread(0xd8, 0x76, 6);
  spread(0xd9, 0x76, 6);
  spread(0xdc, 0x78, 0);
  spread(0xdd, 0x78, 1);
  spread(0xde, 0x77, 2);
  spread(0xdf, 0x77, 3);
  spread(0xe0, 0x77, 3);
  spread(0xe1, 0x76, 4);
  spread(0xe2, 0x76, 4);
  spread(0xe3, 0x76, 5);
  spread(0xe6, 0x81, 0);
  spread(0xe7, 0x81, 1);
  spread(0xe8, 0x81, 2);
  spread(0xe9, 0x80, 3);
  spread(0xea, 0x80, 4);
  spread(0xeb, 0x80, 5);
  spread(0xec, 0x7f, 6);
  spread(0xed, 0x7f, 7);

  std::string const prefix{std::string{zone} + ".WZ/"};
  load(prefix + "TERRAIN.RGB", 71, 64);
  load(prefix + (overcast ? "FOGDULL.RGB" : "FOG.RGB"), 135, 20);
  auto const mapping{data.read(prefix + "GLASS.CMP")};
  if(mapping.size() != glass.size()) throw std::runtime_error{"Invalid glass colour map"};
  std::ranges::copy(mapping, glass.begin());
  glass[155] = 155;
  for(size_t i{156}; i != 168; ++i) glass[i] = static_cast<uint8_t>(i - 1);
  for(size_t i{224}; i != 256; ++i) glass[i] = static_cast<uint8_t>(i);
}
} // namespace apache::render
