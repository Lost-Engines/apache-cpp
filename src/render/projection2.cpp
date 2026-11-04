#include "projection2.h"
#include <stdexcept>
#include "math/fixed.h"

namespace apache::render {
projection2::projection2(rectangle const area, window2 const extent) : viewport{area}, window{extent} {
  int64_t const width{int64_t{window.right} - window.left}, height{int64_t{window.top} - window.bottom};
  if(width <= 0 || height <= 0 || width > INT32_MAX || height > INT32_MAX || viewport.right < viewport.left || viewport.bottom < viewport.top) throw std::runtime_error{"Invalid 2D window dimensions"};
  centre = {static_cast<int32_t>((int64_t{window.left} + window.right) / 2), static_cast<int32_t>((int64_t{window.bottom} + window.top) / 2)};
  int64_t const x_scale{((int64_t{viewport.right} - viewport.left) << 31) / width}, y_scale{((int64_t{viewport.bottom} - viewport.top) << 31) / height};
  if(x_scale > INT32_MAX || y_scale > INT32_MAX) throw std::runtime_error{"2D window magnification exceeds the original fixed-point range"};
  scale = {static_cast<int32_t>(x_scale), -static_cast<int32_t>(y_scale)};
}
uint8_t projection2::outcode(vertex2 const &point) const {
  return static_cast<uint8_t>((point.y > window.top ? 8 : point.y < window.bottom ? 4 : 0) | (point.x > window.right ? 2 : point.x < window.left ? 1 : 0));
}
vertex2 projection2::transform(vertex2 const &point) const {
  int32_t const x{math::add(point.x, math::subtract(origin[0], centre[0]))}, y{math::add(point.y, math::subtract(origin[1], centre[1]))};
  return {math::add(math::rounded_q31(math::product_bits(cosine, x) - math::product_bits(sine, y)), centre[0]), math::add(math::rounded_q31(math::product_bits(cosine, y) + math::product_bits(sine, x)), centre[1]), point.intensity};
}
screen_vertex projection2::screen(vertex2 const &point) const {
  return {math::add(math::multiply_q31(math::subtract(point.x, window.left), scale[0]), viewport.left), math::add(math::multiply_q31(math::subtract(point.y, window.bottom), scale[1]), viewport.bottom), 0, 0, point.intensity};
}
screen_vertex projection2::project(vertex2 const &point) const { return screen(transform(point)); }
bool projection2::clip_endpoint(vertex2 &point, vertex2 const &other) const {
  uint8_t code{outcode(point)};
  auto const other_code{outcode(other)};
  for(unsigned int const plane : {8u, 4u, 2u, 1u}) if(code & plane) {
    bool const horizontal{plane >= 4};
    int32_t const edge{plane == 8 ? window.top : plane == 4 ? window.bottom : plane == 2 ? window.right : window.left};
    int32_t const numerator{math::subtract(edge, horizontal ? point.y : point.x)}, denominator{math::subtract(horizontal ? other.y : other.x, horizontal ? point.y : point.x)};
    if(!denominator) return false;
    int64_t const delta{int64_t{math::subtract(horizontal ? other.x : other.y, horizontal ? point.x : point.y)} * numerator / denominator};
    if(delta < INT32_MIN || delta > INT32_MAX) return false;
    if(horizontal) { point.x = math::add(point.x, static_cast<int32_t>(delta)); point.y = edge; }
    else { point.y = math::add(point.y, static_cast<int32_t>(delta)); point.x = edge; }
    code = outcode(point);
    if((code & plane) || (code & other_code)) return false;
  }
  return !code;
}
std::optional<std::array<screen_vertex, 2>> projection2::clip_line(vertex2 const &a, vertex2 const &b) const {
  auto const first{transform(a)}, last{transform(b)};
  if(outcode(first) & outcode(last)) return std::nullopt;
  auto from{first}, to{last};
  if(!clip_endpoint(from, last) || !clip_endpoint(to, first)) return std::nullopt;
  return std::array{screen(from), screen(to)};
}
std::vector<screen_vertex> projection2::clip(std::span<vertex2 const> const vertices) const {
  if(vertices.empty()) return {};
  std::vector<vertex2> polygon;
  polygon.reserve(vertices.size());
  unsigned int any{0}, all{255};
  for(auto const &vertex : vertices) {
    auto const point{transform(vertex)};
    polygon.push_back(point);
    auto const code{outcode(point)}; any |= code; all &= code;
  }
  if(all) return {};
  for(unsigned int const plane : {8u, 4u, 2u, 1u}) {
    if(!(any & plane)) continue;
    std::vector<vertex2> clipped;
    auto previous{polygon.back()};
    for(auto const &current : polygon) {
      if((outcode(current) ^ outcode(previous)) & plane) {
        bool const horizontal{plane >= 4};
        int32_t const edge{plane == 8 ? window.top : plane == 4 ? window.bottom : plane == 2 ? window.right : window.left};
        int32_t const numerator{math::subtract(edge, horizontal ? current.y : current.x)}, denominator{math::subtract(horizontal ? previous.y : previous.x, horizontal ? current.y : current.x)};
        if(!denominator) return {};
        auto const lerp{[&](int32_t const a, int32_t const b){ return math::add(a, static_cast<int32_t>(int64_t{math::subtract(b, a)} * numerator / denominator)); }};
        clipped.push_back({horizontal ? lerp(current.x, previous.x) : edge, horizontal ? edge : lerp(current.y, previous.y), lerp(current.intensity, previous.intensity)});
      }
      if(!(outcode(current) & plane)) clipped.push_back(current);
      previous = current;
    }
    any = 0; all = 255;
    for(auto const &point : clipped) { auto const code{outcode(point)}; any |= code; all &= code; }
    if(clipped.empty() || (any & plane) || all) return {};
    polygon = std::move(clipped);
  }
  std::vector<screen_vertex> result;
  result.reserve(polygon.size());
  for(auto const &point : polygon) result.push_back(screen(point));
  return result;
}
} // namespace apache::render
