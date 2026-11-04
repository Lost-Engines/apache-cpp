#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include "configuration/preferences.h"
#include "resources/data_store.h"
#include "resources/image.h"
#include "resources/terrain.h"

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
  std::cout << "Resource boundary and configuration tests passed" << std::endl;
  return EXIT_SUCCESS;
} catch(std::exception const &error) {
  std::cerr << error.what() << std::endl;
  return EXIT_FAILURE;
}
