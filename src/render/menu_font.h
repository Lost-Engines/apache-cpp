#pragma once
#include <array>
#include <string_view>
#include "framebuffer.h"

namespace apache::render {
class menu_font {
  std::array<resources::indexed_image, 256> glyphs;
  std::array<unsigned int, 256> advances;

public:
  menu_font(resources::indexed_image const &sheet);
  int width(std::string_view text) const;
  void draw(framebuffer &target, std::string_view text, int x, int y, uint8_t colour, int line_height = 16) const;
};
} // namespace apache::render
