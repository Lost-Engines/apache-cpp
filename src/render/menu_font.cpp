#include "menu_font.h"
#include <algorithm>
#include <stdexcept>

namespace apache::render {
menu_font::menu_font(resources::indexed_image const &sheet) {
  if(sheet.width != 640 || sheet.height != 480) throw std::runtime_error{"Unexpected menu font sheet dimensions"};
  for(size_t c{0}; c != 256; ++c) {
    auto &glyph{glyphs[c]};
    glyph.width = glyph.height = 16;
    glyph.pixels.resize(256);
    unsigned int rightmost{0};
    for(unsigned int y{0}; y != 16; ++y) for(unsigned int x{0}; x != 16; ++x) {
      uint8_t const pixel{sheet.pixels[(c / 32 * 17 + y) * 640 + c % 32 * 17 + x]};
      if(pixel > 1) {
        glyph.pixels[y * 16 + x] = static_cast<uint8_t>(pixel - 1);
        rightmost = std::max(rightmost, x);
      }
    }
    advances[c] = (rightmost ? rightmost : 4) + 1;
  }
  auto const digit{*std::max_element(advances.begin() + '0', advances.begin() + '9' + 1)};
  std::fill(advances.begin() + '0', advances.begin() + '9' + 1, digit);
}
int menu_font::width(std::string_view const text) const {
  int width{0};
  for(unsigned char const c : text) width += static_cast<int>(advances[c]);
  return width;
}
void menu_font::draw(framebuffer &target, std::string_view const text, int x, int y, uint8_t const colour, int const line_height) const {
  int const start{x};
  for(unsigned char const c : text) {
    if(c == '\n') {
      x = start;
      y += line_height;
      continue;
    }
    auto const &glyph{glyphs[c]};
    for(int yy{0}; yy != 16; ++yy) for(int xx{0}; xx != 16; ++xx) if(glyph.pixels[static_cast<size_t>(yy * 16 + xx)]) target.point(x + xx, y + yy, colour);
    x += static_cast<int>(advances[c]);
  }
}
} // namespace apache::render
