#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <set>
#include <stdexcept>
#include <string_view>
#include "math/trigonometry.h"
#include "render/projection.h"
#include "render/rasterizer.h"
#include "resources/data_store.h"
#include "resources/image.h"
#include "resources/models.h"
#include "resources/terrain.h"

namespace {
void word(apache::resources::bytes &output, uint32_t const value) {
  for(unsigned int shift{0}; shift != 32; shift += 8) output.push_back(static_cast<uint8_t>(value >> shift));
}
uint64_t geometry_fingerprint(apache::resources::model const &m) {
  apache::resources::bytes bytes;
  auto const append{[&](auto const value){ word(bytes, static_cast<uint32_t>(value)); }};
  append(m.kind); append(m.lod_distance); append(m.lod_next);
  for(auto const v : m.bounds) append(v);
  append(m.radius); append(m.height);
  for(auto const v : m.offsets) append(v);
  append(m.children.size());
  for(auto const &child : m.children) {
    append(child.model);
    for(auto const v : child.translation) append(v);
    for(auto const v : child.rotation) append(v);
  }
  append(m.vertices.size());
  for(auto const &vertex : m.vertices) for(auto const v : vertex) append(v);
  append(m.normals.size()); append(m.shaded_normals);
  for(auto const &normal : m.normals) for(auto const v : normal) append(v);
  return apache::resources::fingerprint(bytes);
}
void inspect_models(apache::resources::data_store const &data, bool const traces) {
  auto const models{apache::resources::read_models(data.read("3DDATA/3DOBJS.BIN"))};
  for(size_t id{0}; id != models.size(); ++id) {
    if(!models[id]) continue;
    auto const &m{*models[id]};
    if(!traces) {
      std::cout << id << '\t' << m.name << '\t' << m.vertices.size() << '\t' << m.normals.size() << '\t' << m.program.size() << '\t' << std::hex << geometry_fingerprint(m) << std::dec << '\n';
      continue;
    }
    if(!m.offsets[4]) continue;
    std::set<uint32_t> states{0, 1, 2, 4, 8, 15};
    for(auto const &[pc, node] : m.program) if(node.opcode == 5) for(uint32_t i{0}; i != node.branches.size(); ++i) states.insert(i << node.shift);
    for(auto const flags : states) for(int const winding : {1, -1}) {
      apache::resources::bytes events;
      apache::resources::drawing_callbacks const callbacks{
        .face{[&](apache::resources::primitive const&, uint32_t const offset, bool){ word(events, 0); word(events, offset); return winding; }},
        .winding{[&](std::span<uint8_t const>){ return winding; }},
        .child{[&](unsigned int const slot){ word(events, 1); word(events, slot); }},
      };
      try { apache::resources::execute_model(m, flags, winding, callbacks); }
      catch(std::runtime_error const &error) {
        if(std::string_view{error.what()} == "Animation state has no authored case") continue;
        throw;
      }
      std::cout << id << '\t' << flags << '\t' << winding << '\t' << events.size() / 8 << '\t' << std::hex << apache::resources::fingerprint(events) << std::dec << '\n';
    }
  }
}
} // namespace

int main(int const argc, char const *const argv[]) try {
  if(argc < 3) {
    std::cout << "Usage: apache_inspect DATA_DIRECTORY images|terrain|models|model-traces|matrices [RESOURCE [OUTPUT.ppm]]" << std::endl;
    return EXIT_FAILURE;
  }
  apache::resources::data_store const data{argv[1]};
  std::string_view const command{argv[2]};
  if(command == "projection") {
    int x{0}, y{0}, w{0}, h{0}, count{0};
    while(std::cin >> x >> y >> w >> h >> count) {
      apache::render::projection const view{x, y, w, h};
      std::vector<apache::render::vertex3> input;
      for(int i{0}; i != count; ++i) {
        apache::math::vector3 eye;
        int32_t u{0}, v{0}, light{0};
        std::cin >> eye[0] >> eye[1] >> eye[2] >> u >> v >> light;
        input.push_back(view.prepare(eye, u, v, light));
      }
      auto const output{view.clip(input)};
      std::cout << output.size();
      for(auto const &v : output) std::cout << ' ' << v.eye[0] << ' ' << v.eye[1] << ' ' << v.eye[2] << ' ' << v.screen.x << ' ' << v.screen.y << ' ' << v.screen.u << ' ' << v.screen.v << ' ' << v.screen.intensity;
      std::cout << '\n';
    }
  } else if(command == "raster") {
    apache::render::framebuffer frame{64, 48};
    apache::render::rasterizer raster{frame};
    std::array<uint8_t, 256> remapping;
    for(size_t i{0}; i != remapping.size(); ++i) remapping[i] = static_cast<uint8_t>((i * 13 + 7) & 255);
    apache::resources::texture texture{64, apache::resources::bytes(64 * 64)};
    for(size_t i{0}; i != texture.pixels.size(); ++i) texture.pixels[i] = static_cast<uint8_t>((i * 17 + i / 64 * 3) & 255);
    apache::resources::bytes intensity_map(65536);
    for(size_t i{0}; i != intensity_map.size(); ++i) intensity_map[i] = static_cast<uint8_t>((i * 11 + (i >> 8) * 7) & 255);
    int mode{0}, count{0};
    while(std::cin >> mode >> count) {
      std::vector<apache::render::screen_vertex> vertices(static_cast<size_t>(count));
      for(auto &v : vertices) {
        std::cin >> v.x >> v.y;
        if(mode >= 6) std::cin >> v.u >> v.v >> v.intensity;
      }
      for(size_t i{0}; i != frame.image.pixels.size(); ++i) frame.image.pixels[i] = static_cast<uint8_t>((i * 7 + 19) & 255);
      if(mode == 5) raster.line(vertices.at(0), vertices.at(1), 139);
      else if(mode >= 6) {
        apache::render::material paint;
        paint.colours = 0x81468146;
        paint.image = mode == 6 ? nullptr : &texture;
        paint.light = mode == 6 || mode >= 11 ? apache::render::shading::gouraud : mode == 9 || mode == 10 ? apache::render::shading::constant : apache::render::shading::none;
        paint.intensity = 0x58312345;
        paint.intensity_map = intensity_map;
        paint.masked = mode == 8 || mode == 10 || mode == 12;
        raster.fill(vertices, paint);
      } else raster.polygon(vertices, mode == 0 ? 0x81468146 : 0x8b8b8b8b, static_cast<apache::render::fill_pattern>(mode), remapping);
      std::cout << std::hex << apache::resources::fingerprint(frame.image.pixels) << std::dec << '\n';
    }
  } else if(command == "models" || command == "model-traces") inspect_models(data, command == "model-traces");
  else if(command == "matrices") {
    apache::math::trigonometry const trig{data};
    uint32_t heading{0}, pitch{0}, roll{0};
    while(std::cin >> heading >> pitch >> roll) {
      auto const matrix{trig.transformation(heading, pitch, roll)};
      for(auto const v : matrix) std::cout << v << ' ';
      std::cout << '\n';
    }
  } else if(command == "images") {
    for(auto const &name : data.names()) {
      if(argc >= 4 && name != apache::resources::data_store::normalise(argv[3])) continue;
      auto const file{data.read(name)};
      if(file.size() < 4 || std::string_view{reinterpret_cast<char const*>(file.data()), 4} != "FORM") continue;
      auto const image{apache::resources::read_iff(file)};
      std::cout << name << '\t' << image.width << '\t' << image.height << '\t' << std::hex << apache::resources::fingerprint(image.pixels) << std::dec << '\n';
      if(argc >= 5) apache::resources::write_ppm(argv[4], image);
    }
  } else if(command == "terrain") {
    for(std::string_view const zone : {"FORT", "CYPRUS", "KOREA", "YEMEN"}) {
      std::string const prefix{std::string{zone} + ".WZ/"};
      apache::resources::terrain const terrain{data.read(prefix + "GRIDPOST.BIN")};
      auto const objects{apache::resources::read_scenery(data.read(prefix + "GNDOBJS.BIN"))};
      uint64_t checksum{0};
      for(int32_t x{0}; x != 256; ++x) for(int32_t y{0}; y != 256; ++y) checksum = checksum * 0x100000001b3ull + static_cast<uint32_t>(terrain.elevation(x * apache::resources::terrain_step + 500001, y * apache::resources::terrain_step + 700003));
      std::cout << zone << '\t' << objects.size() << '\t' << std::hex << checksum << std::dec << '\n';
    }
  } else throw std::runtime_error{"Unknown inspection command"};
  return EXIT_SUCCESS;
} catch(std::exception const &error) {
  std::cerr << "ERROR: " << error.what() << std::endl;
  return EXIT_FAILURE;
}
