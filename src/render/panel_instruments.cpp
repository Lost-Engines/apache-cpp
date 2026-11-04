#include "panel_instruments.h"
#include <algorithm>
#include <bit>
#include "math/geometry.h"
#include "projection.h"

namespace apache::render {
panel_instruments::panel_instruments(cockpit const &artwork, math::trigonometry const &angles, resources::executable_data const &executable) : panels{artwork}, trig{angles}, font{executable, artwork.video_mode() != 0}, high{artwork.video_mode() != 0} {}
projection2 panel_instruments::dial(framebuffer &target, int const x, int const y, size_t const source_offset) const {
  auto const &background{panels.sprite(high ? "PLT_1.640" : "PLT_1.320")};
  int const size{high ? 51 : 25};
  target.reset_clip();
  for(int row{0}; row != size; ++row) for(int column{0}; column != size; ++column) target.point(x + column, y + row, background.pixels.at(source_offset + static_cast<size_t>(row) * background.width + column));
  target.clip = intersect(target.clip, {x, y, x + size - 1, y + size - 1});
  return {target.clip, {-0x100000, -0x100000, 0x100000, 0x100000}};
}
void panel_instruments::hand(framebuffer &target, projection2 &plane, uint32_t const angle, int32_t const length, uint8_t const colour) const {
  plane.cosine = trig.cosine(angle); plane.sine = trig.sine(angle);
  if(auto const line{plane.clip_line({0, 0}, {0, length})}) rasterizer{target}.line((*line)[0], (*line)[1], colour);
}
void panel_instruments::airspeed(framebuffer &target, int32_t const speed) const {
  auto plane{dial(target, high ? 514 : 257, high ? 428 : 214, high ? 0x1c402 : 0x7181)};
  auto const angle{0u - static_cast<uint32_t>(static_cast<uint64_t>(std::clamp(speed, 0, 250)) * 0xf5555555u / 250)};
  hand(target, plane, angle, 0xc0000, 248);
}
void panel_instruments::altimeter(framebuffer &target, int32_t const altitude) const {
  auto plane{dial(target, high ? 575 : 288, high ? 428 : 214, high ? 0x1c43f : 0x71a0)};
  uint64_t const height{static_cast<uint32_t>(std::max(altitude, 0))};
  hand(target, plane, 0u - static_cast<uint32_t>((height % 256000) * UINT32_MAX / 256000), 0xc0000, 248);
  hand(target, plane, 0u - static_cast<uint32_t>((height % 2560000) * UINT32_MAX / 2560000), 0x80000, 248);
}
void panel_instruments::clock(framebuffer &target, int32_t const seconds) const {
  auto plane{dial(target, high ? 560 : 280, high ? 249 : 124, high ? 0x4b0 : 0x258)};
  auto const angle{[&](int32_t const period){ return 0u - static_cast<uint32_t>(uint64_t{static_cast<uint32_t>(seconds % period)} * UINT32_MAX / period); }};
  hand(target, plane, angle(43200), 0x80000, 248);
  hand(target, plane, angle(3600), 0xc0000, 248);
  hand(target, plane, angle(60), 0xe6666, 249);
}
void panel_instruments::shade(framebuffer &target, std::string_view const name, int const x, int const y, uint8_t const base, uint8_t const excluded) const {
  auto const &mask{panels.sprite(name)};
  for(unsigned int row{0}; row != mask.height; ++row) for(unsigned int column{0}; column != mask.width; ++column) {
    int const xx{x + static_cast<int>(column)}, yy{y + static_cast<int>(row)};
    if(!target.clip.contains(xx, yy) || xx < 0 || yy < 0 || xx >= static_cast<int>(target.image.width) || yy >= static_cast<int>(target.image.height)) continue;
    auto &pixel{target.image.pixels[static_cast<size_t>(yy) * target.image.width + xx]};
    auto const value{mask.pixels[static_cast<size_t>(row) * mask.width + column]};
    if(value && pixel != excluded) pixel = static_cast<uint8_t>(pixel + value - base);
  }
}
void panel_instruments::compass(framebuffer &target, uint32_t const heading) const {
  int const x{high ? 36 : 19}, y{high ? 266 : 133}, width{high ? 37 : 17}, height{high ? 18 : 9};
  target.reset_clip(); target.clip = intersect(target.clip, {x, y, x + width - 1, y + height - 1});
  projection2 const plane{target.clip, {-0x100000, 0, 0x100000, 0x200000}};
  target.fill(target.clip, 225);
  int32_t const spacing{high ? 0x547ae : 0x80000};
  int tick{static_cast<int>(heading / 0x71c71c7u) - 4};
  if(tick < 0) tick += 36;
  int32_t position{-4 * spacing - static_cast<int32_t>(int64_t{heading % 0x71c71c7u} * spacing / 0x71c71c7)};
  rasterizer raster{target};
  for(int i{0}; i != 9; ++i) {
    if(auto const line{plane.clip_line({position, 0x4cccc}, {position, 0x66665})}) raster.line((*line)[0], (*line)[1], 229);
    if(tick % 3 == 0) {
      auto const point{plane.project({position, 0x4cccc})};
      std::string const text{tick == 0 ? "N" : tick == 9 ? "E" : tick == 18 ? "S" : tick == 27 ? "W" : std::to_string(tick)};
      int const offset{text.size() > 1 ? (high ? -5 : -3) : (high ? -2 : -1)};
      font.draw(target, text, point.x + offset, point.y + (high ? -9 : -7), 229);
    }
    if(++tick == 36) tick = 0;
    position += spacing;
  }
  shade(target, high ? "COMSHADE.640" : "COMSHADE.320", x, y, 229, 225);
  target.blit(panels.sprite(high ? "COMBEZEL.640" : "COMBEZEL.320"), x, y, true);
}
void panel_instruments::attitude(framebuffer &target, math::matrix3 const &rotation) const {
  int const x{high ? 548 : 274}, y{high ? 373 : 186}, size{high ? 43 : 22};
  target.reset_clip(); target.clip = intersect(target.clip, {x, y, x + size - 1, y + size - 1});
  auto const angles{trig.angles(rotation)};
  int32_t const pitch{std::bit_cast<int32_t>(angles[1])};
  if(pitch > 0x38e38e39) target.fill(target.clip, 232);
  else if(pitch < -0x38e38e39) target.fill(target.clip, 226);
  else {
    int32_t const half{size * 500};
    projection const view{x, y, size, size};
    projection2 const plane{target.clip, {-half, -half, half, half}};
    auto const matrix{trig.transformation(0, angles[1], angles[2])};
    auto const endpoint{[&](int32_t const local_x){
      auto const eye{view.prepare(math::transform(matrix, {local_x, 0x6400, 0})).eye};
      return vertex2{static_cast<int32_t>(int64_t{eye[0]} * half / eye[1]), static_cast<int32_t>(int64_t{eye[2]} * half / eye[1])};
    }};
    auto const left{endpoint(-0x271000)}, right{endpoint(0x271000)};
    int32_t const dx{math::multiply_q31(-trig.sine(angles[2]), 0x10000000)}, dy{math::multiply_q31(trig.cosine(angles[2]), 0x10000000)};
    std::array<vertex2, 4> const ground{{{math::subtract(left.x, dx), math::subtract(left.y, dy)}, left, right, {math::subtract(right.x, dx), math::subtract(right.y, dy)}}};
    std::array<vertex2, 4> const sky{{left, {math::add(left.x, dx), math::add(left.y, dy)}, {math::add(right.x, dx), math::add(right.y, dy)}, right}};
    rasterizer raster{target}; raster.polygon(plane.clip(ground), 0xe2e2e2e2); raster.polygon(plane.clip(sky), 0xe8e8e8e8);
  }
  shade(target, high ? "HOZSHADE.640" : "HOZSHADE.320", x, y, 224, 0);
  target.blit(panels.sprite(high ? "HOZBEZEL.640" : "HOZBEZEL.320"), x, y, true);
}
} // namespace apache::render
