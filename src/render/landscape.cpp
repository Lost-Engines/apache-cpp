#include "landscape.h"
#include <algorithm>
#include <cstdlib>
#include <stdexcept>
#include <string>
#include "terrain_projection.h"

namespace apache::render {
landscape::landscape(resources::data_store const &data, std::string_view const zone) : terrain{data.read(std::string{zone} + ".WZ/GRIDPOST.BIN")}, textures{data, zone}, features{data, zone, false}, trig{data}, palette{data, zone} {}
void landscape::draw(framebuffer &target, math::pose camera, landscape_options const &options) {
  if(options.terrain_detail > 3) throw std::runtime_error{"Terrain quality must be between zero and three"};
  projection const view{target.clip.left, target.clip.top, target.clip.right - target.clip.left + 1, target.clip.bottom - target.clip.top + 1};
  horizon background{target, target.clip, palette, trig};
  terrain_fog const fog{options.distance};
  auto const elevation{terrain.elevation(camera.position[0], camera.position[1])};
  if(camera.position[2] <= elevation) camera.position[2] = elevation + 256;
  auto const angles{trig.angles(camera.rotation)};
  background.ground(angles[1], angles[2], options.sky);
  feature_renderer feature_drawer{target, view, palette};
  terrain_renderer terrain_drawer{target, view, palette, textures};
  draw_queue queue;
  struct tile_draw {
    unsigned int x{0};
    unsigned int y{0};
    unsigned int half{0};
    terrain_method method{terrain_method::constant};
  };
  std::vector<tile_draw> tiles;
  std::vector<vertex3> grid;
  int grid_left{0}, grid_bottom{0};
  size_t rows{0};
  auto const corners{[&](unsigned int const x, unsigned int const y){
    size_t const index{(x - static_cast<unsigned int>(grid_left)) * rows + y - static_cast<unsigned int>(grid_bottom)};
    return std::array<vertex3, 4>{grid[index], grid[index + 1], grid[index + rows], grid[index + rows + 1]};
  }};
  auto const draw_tile{[&](tile_draw const &tile, bool const two_sided){ terrain_drawer.draw(terrain.post(tile.x, tile.y), corners(tile.x, tile.y), tile.half, tile.method, fog, two_sided); }};
  auto const draw_feature{[&](resources::feature_layer const &layer, int const x, int const y, math::vector3 const &origin, bool const distant){
    if(!options.features) return;
    size_t const index{static_cast<size_t>(x * (layer.super ? 16 : 64) + y)};
    auto const id{layer.placements.at(index)};
    if(!id) return;
    auto const &object{*layer.records[id]};
    auto const points{project_feature(object, camera.rotation, origin, view)};
    feature_drawer.draw(distant && object.has_distant ? object.distant : object.faces, points, fog);
  }};
  if(options.sky.cloud_mode == 0 || (!(options.sky.cloud_flags & 2) && options.sky.weather == 0)) {
    // The reference engine projects the super-feature vertices but never
    // submits their face lists. They remain available in the resource tools.
    auto const sectors{scan_sectors(camera, view, angles[0], options.distance, false)};
    if(!sectors.empty()) {
      int const sx{camera.position[0] >> 22}, sy{camera.position[1] >> 22};
      grid_left = (sx + sectors.left) * 4; grid_bottom = (sy + sectors.bottom) * 4;
      rows = static_cast<size_t>((sectors.top - sectors.bottom + 1) * 4 + 1);
      grid = project_terrain(terrain, camera, view, grid_left, grid_bottom, static_cast<unsigned int>((sectors.right - sectors.left + 1) * 4 + 1), static_cast<unsigned int>(rows));
      unsigned int const detail{options.distance < 4 ? std::min(options.terrain_detail, 1u) : options.terrain_detail};
      for(int x{sectors.left}; x <= sectors.right; ++x) for(int y{sectors.bottom}; y <= sectors.top; ++y) {
        auto const origin{feature_origin(camera, sx + x, sy + y, false)};
        if(!sector_visibility(camera.rotation, origin, view, false)) continue;
        for(unsigned int gx{static_cast<unsigned int>((sx + x) * 4)}; gx != static_cast<unsigned int>((sx + x + 1) * 4); ++gx) for(unsigned int gy{static_cast<unsigned int>((sy + y) * 4)}; gy != static_cast<unsigned int>((sy + y + 1) * 4); ++gy) {
          auto const points{corners(gx, gy)};
          unsigned int const range{approximate_range(points[0].eye) >> 22};
          if(range >= fog.boundary + fog.width) continue;
          auto const &post{terrain.post(gx, gy)};
          std::array<std::array<unsigned int, 3>, 4> constexpr indices{{{2, 0, 1}, {1, 3, 2}, {0, 1, 3}, {3, 2, 0}}};
          bool const near{camera.position[2] - elevation < 0x200000 && std::abs((camera.position[0] >> 20) - static_cast<int>(gx)) < 3 && std::abs((camera.position[1] >> 20) - static_cast<int>(gy)) < 3};
          unsigned int const level{range < fog.boundary ? detail : 0};
          for(unsigned int half{0}; half != 2; ++half) {
            auto const &order{indices[((post.flags & 64) ? 2 : 0) + half]};
            unsigned int common{255};
            int32_t depth{INT32_MIN}, sum{0};
            for(auto const index : order) { common &= points[index].outcode; depth = std::max(depth, points[index].eye[1]); sum = math::add(sum, points[index].eye[1]); }
            if(common) continue;
            terrain_method method{terrain_method::constant};
            if(detail == 1 && level) method = near ? terrain_method::subdivided : terrain_method::gouraud;
            else if(detail == 2) {
              if(level == 2) method = (post.textures[half] & 127) == 127 ? (near ? terrain_method::subdivided : terrain_method::gouraud) : (near ? terrain_method::subdivided_texture : terrain_method::texture32);
              else if(level == 1) method = terrain_method::gouraud;
            } else if(detail == 3) method = near ? terrain_method::perspective : terrain_method::texture64;
            tile_draw const tile{gx, gy, half, method};
            if(post.textures[half] & 128) draw_tile(tile, true);
            else {
              auto const payload{static_cast<uint32_t>(tiles.size())};
              if(queue.push({draw_kind::terrain, payload, depth, sum, false})) tiles.push_back(tile);
            }
          }
        }
        draw_feature(features, sx + x, sy + y, origin, std::abs(x) >= 4 || std::abs(y) >= 4);
      }
    }
  }
  background.sky(options.sky);
  for(auto const &entry : queue.ordered()) draw_tile(tiles[entry.payload], false);
}
} // namespace apache::render
