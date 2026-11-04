#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <string_view>
#include "resources/data_store.h"
#include "resources/image.h"
#include "resources/terrain.h"

int main(int const argc, char const *const argv[]) try {
  if(argc < 3) {
    std::cout << "Usage: apache_inspect DATA_DIRECTORY images|terrain [RESOURCE [OUTPUT.ppm]]" << std::endl;
    return EXIT_FAILURE;
  }
  apache::resources::data_store const data{argv[1]};
  std::string_view const command{argv[2]};
  if(command == "images") {
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
