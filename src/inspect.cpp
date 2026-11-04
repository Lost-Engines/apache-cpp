#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <set>
#include <stdexcept>
#include <string_view>
#include "math/trigonometry.h"
#include "render/draw_queue.h"
#include "render/feature_renderer.h"
#include "render/horizon.h"
#include "render/landscape.h"
#include "render/model_renderer.h"
#include "render/projection.h"
#include "render/projection2.h"
#include "render/rasterizer.h"
#include "render/sector_scan.h"
#include "render/terrain_projection.h"
#include "render/terrain_renderer.h"
#include "render/terrain_shading.h"
#include "resources/data_store.h"
#include "resources/image.h"
#include "resources/models.h"
#include "resources/terrain.h"
#include "resources/terrain_textures.h"

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
  if(command == "landscape") {
    apache::math::trigonometry const trig{data};
    std::string zone, loaded;
    std::unique_ptr<apache::render::landscape> world;
    apache::math::pose camera;
    uint32_t heading{0}, pitch{0}, roll{0};
    apache::render::landscape_options options;
    while(std::cin >> zone >> camera.position[0] >> camera.position[1] >> camera.position[2] >> heading >> pitch >> roll >> options.distance >> options.terrain_detail >> options.features) {
      if(!world || loaded != zone) { world = std::make_unique<apache::render::landscape>(data, zone); loaded = zone; }
      apache::render::framebuffer frame{320, 240};
      for(size_t i{0}; i != 256; ++i) frame.image.colours[i] = {world->palette.colours[i * 3], world->palette.colours[i * 3 + 1], world->palette.colours[i * 3 + 2]};
      frame.clear(255);
      camera.rotation = trig.transformation(heading, pitch, roll);
      world->draw(frame, camera, options);
      std::cout << std::hex << apache::resources::fingerprint(frame.image.pixels) << std::dec << '\n';
      if(argc > 3) { apache::resources::write_ppm(argv[3], frame.image); std::ofstream file{std::string{argv[3]} + ".pixels", std::ios::binary}; file.write(reinterpret_cast<char const *>(frame.image.pixels.data()), static_cast<std::streamsize>(frame.image.pixels.size())); }
    }
  } else if(command == "horizon") {
    apache::math::trigonometry const trig{data};
    apache::render::scene_palette const palette{data, "FORT"};
    int w{0}, h{0};
    uint32_t pitch{0}, roll{0};
    unsigned int ground{0};
    apache::render::sky_conditions conditions;
    while(std::cin >> w >> h >> pitch >> roll >> conditions.cloud_mode >> conditions.cloud_flags >> conditions.weather >> ground) {
      apache::render::framebuffer frame{static_cast<unsigned int>(w), static_cast<unsigned int>(h)};
      frame.clear(255); conditions.ground_colour = static_cast<uint8_t>(ground);
      apache::render::horizon horizon{frame, frame.clip, palette, trig};
      horizon.ground(pitch, roll, conditions); horizon.sky(conditions);
      std::cout << apache::resources::fingerprint(frame.image.pixels) << '\n';
      if(argc > 3) { std::ofstream file{argv[3], std::ios::binary}; file.write(reinterpret_cast<char const *>(frame.image.pixels.data()), static_cast<std::streamsize>(frame.image.pixels.size())); }
    }
  } else if(command == "projection2") {
    apache::math::trigonometry const trig{data};
    apache::render::window2 window;
    apache::render::rectangle viewport;
    int32_t origin_x{0}, origin_y{0};
    uint32_t angle{0};
    size_t count{0};
    while(std::cin >> viewport.left >> viewport.top >> viewport.right >> viewport.bottom >> window.left >> window.bottom >> window.right >> window.top >> origin_x >> origin_y >> angle >> count) {
      apache::render::projection2 view{viewport, window};
      view.origin = {origin_x, origin_y}; view.cosine = trig.cosine(angle); view.sine = trig.sine(angle);
      std::vector<apache::render::vertex2> polygon(count);
      for(auto &point : polygon) std::cin >> point.x >> point.y >> point.intensity;
      auto const result{view.clip(polygon)};
      std::cout << result.size();
      for(auto const &point : result) std::cout << ' ' << point.x << ' ' << point.y << ' ' << point.intensity;
      std::cout << '\n';
    }
  } else if(command == "sectors") {
    apache::math::trigonometry const trig{data};
    apache::render::projection const view{0, 0, 320, 240};
    std::string zone;
    apache::math::pose camera;
    uint32_t heading{0}, pitch{0}, roll{0}, distance{0};
    int sector_x{0}, sector_y{0};
    bool super{false};
    while(std::cin >> zone >> camera.position[0] >> camera.position[1] >> camera.position[2] >> heading >> pitch >> roll >> distance >> sector_x >> sector_y >> super) {
      apache::resources::terrain const terrain{data.read(zone + ".WZ/GRIDPOST.BIN")};
      auto const ground{terrain.elevation(camera.position[0], camera.position[1])};
      if(camera.position[2] <= ground) camera.position[2] = ground + 256;
      camera.rotation = trig.transformation(heading, pitch, roll);
      for(bool const layer : {false, true}) {
        auto const bounds{apache::render::scan_sectors(camera, view, trig.angles(camera.rotation)[0], distance, layer)};
        std::cout << bounds.left << ' ' << bounds.bottom << ' ' << bounds.right << ' ' << bounds.top << ' ';
      }
      auto const origin{apache::render::feature_origin(camera, sector_x, sector_y, super)};
      std::cout << apache::render::sector_visibility(camera.rotation, origin, view, super);
      for(auto const value : origin) std::cout << ' ' << value;
      std::cout << '\n';
    }
  } else if(command == "inverse-trig") {
    apache::math::trigonometry const trig{data};
    uint32_t heading{0}, pitch{0}, roll{0};
    int32_t x{0}, y{0};
    while(std::cin >> heading >> pitch >> roll >> x >> y) {
      auto const angles{trig.angles(trig.transformation(heading, pitch, roll))};
      for(auto const angle : angles) std::cout << angle << ' ';
      std::cout << trig.arctangent(x, y) << '\n';
    }
  } else if(command == "features") {
    for(std::string_view const zone : {"FORT", "CYPRUS", "KOREA", "YEMEN"}) for(bool const super : {false, true}) {
      apache::resources::feature_layer const layer{data, zone, super};
      for(size_t id{1}; id != layer.records.size(); ++id) if(layer.records[id]) {
        auto const &object{*layer.records[id]};
        apache::resources::bytes record;
        word(record, object.vertices.size());
        for(auto const &v : object.vertices) {
          word(record, v.flags);
          std::array<int32_t, 2> position{0, 0};
          std::array<int32_t, 4> constexpr units{1 << 26, 1 << 18, 1 << 12, 1 << 6};
          for(size_t level{0}; level != 4; ++level) for(size_t axis{0}; axis != 2; ++axis) position[axis] += v.components[level][axis] * units[level];
          for(auto const value : position) word(record, static_cast<uint32_t>(value));
        }
        for(auto const *list : {&object.faces, &object.distant}) {
          word(record, list->size());
          for(auto const &face : *list) {
            word(record, face.type); word(record, face.surface); word(record, face.colours[0]); word(record, face.colours[1]);
            for(auto const *values : {&face.indices, &face.intensities, &face.render_indices}) {
              word(record, values->size());
              for(auto const value : *values) word(record, value);
            }
          }
        }
        std::cout << zone << ' ' << super << ' ' << id << ' ' << std::hex << apache::resources::fingerprint(record) << std::dec << '\n';
      }
    }
  } else if(command == "feature-frames") {
    std::string zone;
    bool super{false}, distant{false};
    uint32_t id{0}, heading{0}, pitch{0}, roll{0}, detail{0};
    apache::math::vector3 origin;
    apache::math::trigonometry const trig{data};
    apache::render::projection const view{0, 0, 320, 240};
    std::string loaded;
    std::optional<apache::resources::feature_layer> layer;
    std::optional<apache::render::scene_palette> palette;
    apache::render::framebuffer frame{320, 240};
    while(std::cin >> zone >> super >> id >> distant >> detail >> heading >> pitch >> roll >> origin[0] >> origin[1] >> origin[2]) {
      std::string const key{zone + (super ? ":super" : ":normal")};
      if(key != loaded) { layer.emplace(data, zone, super); palette.emplace(data, zone); loaded = key; }
      if(id >= layer->records.size() || !layer->records[id]) throw std::runtime_error{"Invalid feature record"};
      auto const &object{*layer->records[id]};
      auto const points{apache::render::project_feature(object, trig.transformation(heading, pitch, roll), origin, view)};
      apache::render::feature_renderer renderer{frame, view, *palette};
      frame.clear(155);
      renderer.draw(distant && object.has_distant ? object.distant : object.faces, points, apache::render::terrain_fog{detail});
      apache::resources::bytes coordinates;
      for(auto const &point : points) for(int32_t const value : {point.eye[0], point.eye[1], point.eye[2], point.screen.x, point.screen.y, int32_t{point.outcode}}) word(coordinates, static_cast<uint32_t>(value));
      std::cout << std::hex << apache::resources::fingerprint(coordinates) << ' ' << apache::resources::fingerprint(frame.image.pixels) << std::dec << '\n';
      if(argc >= 4) {
        std::ofstream raw{argv[3], std::ios::binary}; raw.write(reinterpret_cast<char const*>(frame.image.pixels.data()), static_cast<std::streamsize>(frame.image.pixels.size()));
        for(auto const &point : points) std::cerr << point.eye[0] << ' ' << point.eye[1] << ' ' << point.eye[2] << ' ' << point.screen.x << ' ' << point.screen.y << ' ' << +point.outcode << '\n';
      }
    }
  } else if(command == "terrain-polygons") {
    apache::render::scene_palette const palette{data, "FORT"};
    apache::resources::terrain_textures const textures{data, "FORT"};
    apache::render::projection const view{0, 0, 320, 240};
    apache::render::framebuffer frame{320, 240};
    apache::render::terrain_renderer renderer{frame, view, palette, textures};
    unsigned int method{0}, detail{0}, flags{0}, colour{0}, selector{0}, half{0}, two_sided{0};
    while(std::cin >> method >> detail >> flags >> colour >> selector >> half >> two_sided) {
      std::array<apache::render::vertex3, 4> points;
      for(auto &point : points) {
        apache::math::vector3 eye;
        int32_t light{0};
        std::cin >> eye[0] >> eye[1] >> eye[2] >> light;
        point = view.prepare_scaled(eye, light);
      }
      apache::resources::terrain_post post;
      post.flags = static_cast<uint8_t>(flags);
      post.colours.fill(static_cast<uint8_t>(colour));
      post.textures.fill(static_cast<uint8_t>(selector));
      frame.clear(155);
      renderer.draw(post, points, half, static_cast<apache::render::terrain_method>(method), apache::render::terrain_fog{detail}, two_sided != 0);
      std::cout << std::hex << apache::resources::fingerprint(frame.image.pixels) << std::dec << ' ' << renderer.invalid_texture_samples() << '\n';
      if(argc >= 4) {
        std::ofstream raw{argv[3], std::ios::binary};
        raw.write(reinterpret_cast<char const*>(frame.image.pixels.data()), static_cast<std::streamsize>(frame.image.pixels.size()));
      }
    }
  } else if(command == "terrain-shades") {
    unsigned int detail{0}, colour{0}, kind{0}, salt{0};
    uint32_t range{0};
    while(std::cin >> detail >> range >> colour >> kind >> salt) {
      apache::render::terrain_fog const fog{detail};
      std::array<uint8_t, 256> remap;
      for(unsigned int i{0}; i != 256; ++i) remap[i] = static_cast<uint8_t>(salt ? i * 73 + salt : i);
      std::cout << fog.constant_colour(range, static_cast<uint8_t>(colour), remap) << ' ' << fog.shaded_colours(range, kind, remap) << ' ' << fog.texture_bank(range, 3) << '\n';
    }
  } else if(command == "terrain-textures") {
    for(std::string_view const zone : {"FORT", "CYPRUS", "KOREA", "YEMEN"}) {
      apache::resources::terrain_textures const textures{data, zone};
      for(bool const detailed : {false, true}) for(bool const forward : {false, true}) for(unsigned int selector{0}; selector != 128; ++selector) {
        auto const &texture{textures.get(forward, selector, detailed)};
        std::cout << zone << ' ' << detailed << ' ' << forward << ' ' << selector << ' ' << std::hex << apache::resources::fingerprint(texture.pixels) << std::dec << '\n';
      }
    }
  } else if(command == "fog-tables") {
    apache::render::scene_palette const palette{data, "FORT"};
    for(auto const &table : palette.fog_intensity) {
      std::cout << std::hex << apache::resources::fingerprint(table) << std::dec << '\n';
      if(argc >= 4) { std::ofstream out{std::string{argv[3]} + std::to_string(&table - palette.fog_intensity.data()), std::ios::binary}; out.write(reinterpret_cast<char const*>(table.data()), static_cast<std::streamsize>(table.size())); }
    }
  } else if(command == "terrain-vertices") {
    apache::math::trigonometry const trig{data};
    apache::render::projection const view{0, 0, 320, 240};
    std::string zone;
    apache::math::pose camera;
    uint32_t heading{0}, pitch{0}, roll{0};
    int left{0}, bottom{0};
    unsigned int columns{0}, rows{0};
    while(std::cin >> zone >> camera.position[0] >> camera.position[1] >> camera.position[2] >> heading >> pitch >> roll >> left >> bottom >> columns >> rows) {
      apache::resources::terrain const terrain{data.read(zone + ".WZ/GRIDPOST.BIN")};
      camera.rotation = trig.transformation(heading, pitch, roll);
      auto const ground{terrain.elevation(camera.position[0], camera.position[1])};
      if(camera.position[2] <= ground) camera.position[2] = ground + 256;
      auto const vertices{apache::render::project_terrain(terrain, camera, view, left, bottom, columns, rows)};
      apache::resources::bytes output;
      for(auto const &v : vertices) for(int32_t const field : {v.eye[0], v.eye[1], v.eye[2], v.screen.x, v.screen.y, v.screen.intensity, int32_t{v.outcode}}) {
        for(unsigned int shift{0}; shift != 32; shift += 8) output.push_back(static_cast<uint8_t>(static_cast<uint32_t>(field) >> shift));
        if(argc >= 4) std::cerr << field << ' ';
      }
      std::cout << std::hex << apache::resources::fingerprint(output) << std::dec << '\n';
    }
  } else if(command == "draw-order") {
    unsigned int count{0};
    while(std::cin >> count) {
      apache::render::draw_queue queue;
      for(unsigned int i{0}; i != count; ++i) {
        apache::render::queued_draw draw;
        unsigned int kind{0};
        std::cin >> kind >> draw.depth >> draw.terrain_depth_sum >> draw.flat;
        draw.kind = static_cast<apache::render::draw_kind>(kind);
        draw.payload = i;
        queue.push(draw);
      }
      auto const ordered{queue.ordered()};
      std::cout << ordered.size();
      for(auto const &draw : ordered) std::cout << ' ' << draw.payload;
      std::cout << '\n';
    }
  } else if(command == "palette") {
    for(std::string_view const zone : {"FORT", "CYPRUS", "KOREA", "YEMEN"}) {
      apache::render::scene_palette const palette{data, zone};
      std::cout << zone << ' ' << std::hex << apache::resources::fingerprint(palette.colours) << ' ' << apache::resources::fingerprint(palette.intensity) << ' ' << apache::resources::fingerprint(palette.glass) << std::dec << '\n';
    }
  } else if(command == "geometry") {
    apache::math::matrix3 a, b;
    apache::math::vector3 v;
    while(std::cin >> a[0]) {
      for(size_t i{1}; i != 9; ++i) std::cin >> a[i];
      for(auto &value : b) std::cin >> value;
      for(auto &value : v) std::cin >> value;
      for(auto const value : apache::math::multiply(a, b)) std::cout << value << ' ';
      for(auto const value : apache::math::transform(a, v)) std::cout << value << ' ';
      for(auto const shift : {19u, 25u}) for(auto const value : apache::math::subdivide_grid(a[0], shift)) std::cout << value << ' ';
      std::cout << '\n';
    }
  } else if((command == "model-renders" || command == "model-renders-alternate")) {
    auto const models{apache::resources::read_models(data.read("3DDATA/3DOBJS.BIN"))};
    auto const textures{apache::resources::read_object_textures(data.read("3DDATA/OBJTEXT.BIN"))};
    apache::render::scene_palette const palette{data, "FORT"};
    apache::math::trigonometry const trig{data};
    apache::render::framebuffer frame{320, 240};
    for(size_t i{0}; i != 256; ++i) frame.image.colours[i] = {palette.colours[i * 3], palette.colours[i * 3 + 1], palette.colours[i * 3 + 2]};
    apache::render::model_renderer renderer{frame, models, textures, palette};
    if(argc >= 5 && std::string_view{argv[4]} == "trace") renderer.face_observer = [&](uint32_t const model, uint32_t const pc){
      std::cerr << model << ' ' << pc << ' ' << std::hex << apache::resources::fingerprint(frame.image.pixels) << std::dec << '\n';
    };
    uint32_t id{0}, flags{0}, heading{0}, pitch{0}, roll{0}, camera_heading{0}, camera_pitch{0}, camera_roll{0}, quality{0};
    apache::math::vector3 position;
    while(std::cin >> id >> flags >> position[0] >> position[1] >> position[2] >> heading >> pitch >> roll >> camera_heading >> camera_pitch >> camera_roll >> quality) {
      renderer.style.textures = (quality & 1) != 0;
      renderer.style.gouraud = (quality & 2) != 0;
      auto object{apache::render::construct_model(models, id, {position, trig.transformation(heading, pitch, roll)})};
      object.flags = flags;
      object.alternate_colours = command == "model-renders-alternate";
      apache::math::pose const camera{{0, 0, 1000000}, trig.transformation(camera_heading, camera_pitch, camera_roll)};
      frame.clear(155);
      try {
        renderer.draw(object, camera);
        std::cout << std::hex << apache::resources::fingerprint(frame.image.pixels) << std::dec << ' ' << renderer.missing_normals << ' ' << renderer.invalid_texture_samples << '\n';
      } catch(std::runtime_error const &error) { std::cout << "error " << error.what() << '\n'; }
      if(argc >= 4) {
        apache::resources::write_ppm(argv[3], frame.image);
        std::ofstream raw{std::string{argv[3]} + ".pixels", std::ios::binary};
        raw.write(reinterpret_cast<char const*>(frame.image.pixels.data()), static_cast<std::streamsize>(frame.image.pixels.size()));
      }
    }
  } else if(command == "lines") {
    int x{0}, y{0}, w{0}, h{0};
    while(std::cin >> x >> y >> w >> h) {
      apache::math::vector3 a, b;
      for(auto &value : a) std::cin >> value;
      for(auto &value : b) std::cin >> value;
      apache::render::projection const view{x, y, w, h};
      auto const result{view.clip_line(view.prepare(a), view.prepare(b))};
      std::cout << bool(result);
      if(result) for(auto const &v : *result) std::cout << ' ' << v.eye[0] << ' ' << v.eye[1] << ' ' << v.eye[2] << ' ' << v.screen.x << ' ' << v.screen.y;
      std::cout << '\n';
    }
  } else if(command == "projection") {
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
        if(mode >= 6 && mode <= 12) std::cin >> v.u >> v.v >> v.intensity;
      }
      for(size_t i{0}; i != frame.image.pixels.size(); ++i) frame.image.pixels[i] = static_cast<uint8_t>((i * 7 + 19) & 255);
      if(mode == 5) raster.line(vertices.at(0), vertices.at(1), 139);
      else if(mode >= 13) {
        apache::render::material paint;
        paint.pattern = apache::render::fill_pattern::heat_haze;
        paint.heat_phase = static_cast<uint8_t>(mode - 13);
        raster.fill(vertices, paint);
      } else if(mode >= 6) {
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
