#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include "configuration/preferences.h"
#include "math/fixed.h"
#include "render/celestial.h"
#include "render/draw_queue.h"
#include "render/framebuffer.h"
#include "render/projection.h"
#include "render/projection2.h"
#include "resources/data_store.h"
#include "resources/image.h"
#include "resources/models.h"
#include "resources/terrain.h"
#include "resources/texture.h"

namespace {
void require(bool const condition, char const *const message) {
  if(!condition) throw std::runtime_error{message};
}
template<typename F> void rejects(F const &fn, char const *const message) {
  bool rejected{false};
  try {
    fn();
  } catch(std::runtime_error const&) {
    rejected = true;
  }
  require(rejected, message);
}
} // anonymous namespace

int main() try {
  apache::configuration::preferences options;
  options.load("Key=first\r\nKEY=last\n Spaced =preserved\r\nNumber= -17tail\r\n");
  require(options.get("key") == "last", "last case-insensitive preference wins");
  require(!options.get("Spaced"), "preference key spaces are significant");
  require(options.integer("Number") == -17, "numeric preferences accept a decimal prefix");
  options.set("Key", 42);
  require(options.integer("kEY") == 42, "existing preference update");
  options.set("Big", "999999999999999999999999");
  require(options.integer("Big") == 2147483647, "oversized preferences are bounded");
  auto const serialised{options.serialise()};
  require(serialised.starts_with("Big="), "new preference prepends");
  apache::resources::bytes const packed{128, 2, 1, 2, 3, 254, 7};
  apache::resources::reader stream{packed};
  require(apache::resources::read_byterun(stream, 6) == apache::resources::bytes{1, 2, 3, 7, 7, 7}, "ByteRun literals, repeat and no-op");
  rejects([&]{ apache::resources::reader short_stream{packed}; apache::resources::read_byterun(short_stream, 5); }, "oversized ByteRun rejected");
  rejects([]{ apache::resources::read_offset_table(apache::resources::bytes{255, 255, 255, 255}); }, "oversized offset table rejected");
  rejects([]{ apache::resources::data_store::normalise("../PREFS.CFG"); }, "parent path rejected");
  require(apache::resources::data_store::normalise("3ddata\\objtext.bin") == "3DDATA/OBJTEXT.BIN", "DOS path normalisation");
  apache::resources::bytes grid(257 * 257 * 8);
  for(size_t i{0}; i != 257 * 257; ++i) grid[i * 8] = 100;
  apache::resources::terrain const flat{grid};
  require(flat.elevation(12345678, 98765432) == 25600, "flat terrain interpolation");
  require(flat.elevation(-1, 0) == 0 && flat.elevation(256 * apache::resources::terrain_step, 0) == 0, "terrain bounds");
  apache::render::framebuffer buffer{4, 3};
  buffer.clear(9);
  apache::resources::indexed_image source;
  source.width = 3;
  source.height = 2;
  source.pixels = {1, 0, 2, 3, 4, 5};
  buffer.clip = {1, 1, 9, 9};
  buffer.blit(source, 0, 1, true);
  require(buffer.image.pixels == apache::resources::bytes{9, 9, 9, 9, 9, 9, 2, 9, 9, 4, 5, 9}, "transparent blit clips without losing source alignment");
  buffer.fill({-100, -100, 100, 100}, 7);
  require(buffer.image.pixels == apache::resources::bytes{9, 9, 9, 9, 9, 7, 7, 7, 9, 7, 7, 7}, "fill intersects the framebuffer and caller clip");
  apache::render::rasterizer circles{buffer};
  apache::render::material circle_paint;
  circle_paint.colours = 0x05050505;
  circles.circle(0, 0, 10, circle_paint);
  require(buffer.image.pixels == apache::resources::bytes{9, 9, 9, 9, 9, 5, 5, 5, 9, 5, 5, 5}, "circle clips against both the image and a larger caller viewport");
  circles.circle(INT32_MIN, INT32_MAX, 20, circle_paint);
  rejects([&]{ circles.circle(0, 0, INT32_MAX, circle_paint); }, "oversized circle rejected without arithmetic overflow");
  rejects([]{ apache::render::read_stars(apache::resources::bytes{0, 32, 32}); }, "truncated star record rejected");
  rejects([]{ apache::render::read_stars(apache::resources::bytes{24, 32, 32, 32}); }, "star palette index bounded");
  rejects([]{ apache::render::read_stars(apache::resources::bytes{0, 32, 65, 32}); }, "star grid lookup bounded");
  require(apache::math::rounded_q31(0x40000000ull) == 1, "positive half rounds upward");
  require(apache::math::rounded_q31(0xffffffffc0000000ull) == 0, "negative half rounds toward zero");
  require(apache::math::negate(INT32_MIN) == INT32_MIN, "fixed-point negation has defined wrapping");
  apache::resources::bytes empty_model(188);
  empty_model[68] = 188;
  auto object{apache::resources::read_model(empty_model)};
  require(object.grid_lighting.empty(), "empty lighting section can point to the end of a model");
  empty_model[68] = 189;
  rejects([&]{ apache::resources::read_model(empty_model); }, "model section beyond record rejected");
  object.offsets[4] = 200;
  apache::resources::drawing_node select;
  select.opcode = 5;
  select.mask = 3;
  select.branches = {300};
  select.next = 400;
  object.program.emplace(200, select);
  object.program.emplace(300, apache::resources::drawing_node{});
  object.program.emplace(400, apache::resources::drawing_node{});
  apache::resources::execute_model(object, 0, 1, {});
  rejects([&]{ apache::resources::execute_model(object, 1, 1, {}); }, "unauthored model animation state rejected");
  object.program.at(200).branches[0] = 200;
  rejects([&]{ apache::resources::execute_model(object, 0, 1, {}); }, "cyclic model program bounded");
  apache::render::draw_queue queue;
  for(unsigned int i{0}; i != 7200; ++i) require(queue.consume_unused_slot(), "discarded cloud entries still consume the original buffer capacity");
  require(queue.full() && queue.ordered().empty() && !queue.push({}), "consumed queue slots cannot be rendered or reused before reset");
  queue.clear();
  require(queue.push({}) && queue.ordered().size() == 1, "frame reset releases discarded slots");
  apache::render::projection const camera{0, 0, 320, 240};
  std::array<apache::render::vertex3, 3> const crossing_triangle{camera.prepare({-100, 100, 0}), camera.prepare({100, 100, 0}), camera.prepare({0, -100, 100})};
  auto const clipped{camera.clip(crossing_triangle)};
  require(!clipped.empty(), "near-plane crossing retains the visible polygon");
  for(auto const &v : clipped) require(v.outcode == 0 && v.eye[1] >= 1 && v.screen.x >= 0 && v.screen.x < 320 && v.screen.y >= 0 && v.screen.y < 240, "clipped vertices stay inside the viewport");
  std::array<apache::render::vertex3, 3> const behind{camera.prepare({0, -10, 0}), camera.prepare({1, -10, 0}), camera.prepare({0, -10, 1})};
  require(camera.clip(behind).empty(), "wholly hidden polygon rejected");
  require(apache::render::winding(behind, true) == 0, "coincident screen points cannot loop in robust winding");
  rejects([]{ apache::render::projection{0, 0, 640, 0}; }, "zero-height viewport rejected");
  rejects([]{ apache::render::projection2{{0, 0, 319, 239}, {0, 0, 0, 1000}}; }, "zero-width 2D window rejected");
  rejects([]{ apache::render::projection2{{INT32_MIN, 0, INT32_MAX, 239}, {-1000, -1000, 1000, 1000}}; }, "oversized 2D viewport rejected without overflow");
  apache::render::projection2 const window{{0, 0, 319, 239}, {-1000, -1000, 1000, 1000}};
  std::array<apache::render::vertex2, 4> const surrounding{{{-2000, -2000, 0}, {-2000, 2000, INT32_MAX}, {2000, 2000, INT32_MAX}, {2000, -2000, 0}}};
  auto const window_clip{window.clip(surrounding)};
  require(window_clip.size() == 4, "2D clipping keeps a polygon surrounding the entire window");
  for(auto const &point : window_clip) require(point.x >= 0 && point.x < 320 && point.y >= 0 && point.y < 240 && point.intensity > 0 && point.intensity < INT32_MAX, "2D clipped coordinates and intensities stay inside their bounds");
  apache::resources::bytes bad_texture{1, 0, 0, 0, 8, 0, 0, 0, 2, 0, 0, 0, 64, 0, 0, 0, 0, 0, 0, 0};
  rejects([&]{ apache::resources::read_object_textures(bad_texture); }, "truncated texture pixels rejected");
  std::cout << "Resource boundary and configuration tests passed" << std::endl;
  return EXIT_SUCCESS;
} catch(std::exception const &error) {
  std::cerr << error.what() << std::endl;
  return EXIT_FAILURE;
}
