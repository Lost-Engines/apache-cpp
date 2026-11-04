#pragma once
#include <map>
#include <string>
#include <string_view>
#include <vector>
#include "render/framebuffer.h"
#include "render/menu_font.h"
#include "resources/data_store.h"
#include "widget.h"

namespace apache::frontend {
class menu {
  resources::data_store const &data;
  render::menu_font font;
  std::vector<widget> widgets{initial_widgets()};
  std::map<std::string, resources::indexed_image, std::less<>> images;
  std::map<std::string, resources::indexed_image, std::less<>> sprites;
  std::map<std::string, size_t, std::less<>> globals;
  std::vector<size_t> screens;
  std::vector<render::rectangle> clips;
  int hovered{-1};
  int pressed{-1};
  bool quitting{false};
  resources::indexed_image const &image(std::string const &name);
  void draw_widget(render::framebuffer &target, widget const &object);
  void activate(size_t index);
  int hit(int x, int y) const;

public:
  menu(resources::data_store const &data);
  void show(std::string_view global);
  void back();
  void move(int x, int y);
  void button(bool down);
  void toggle_quick(bool visible);
  void press_named(std::string_view name);
  void render(render::framebuffer &target);
  bool quit_requested() const;
};
} // namespace apache::frontend
