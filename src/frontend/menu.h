#pragma once
#include <functional>
#include <map>
#include <optional>
#include <span>
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
  int cursor_x{0}, cursor_y{0};
  struct popup_state {
    std::vector<std::pair<std::string, int>> choices;
    std::function<void(int)> selected;
    render::rectangle area;
    int hovered{-1};
  };
  std::optional<popup_state> active_popup;
  int drag_y{0}, drag_position{0}, scroll_direction{0};
  std::vector<std::pair<size_t, size_t>> scroll_links;
  void scroll_changed(size_t index);
  bool quitting{false};
  resources::indexed_image const &image(std::string const &name);
  void draw_widget(render::framebuffer &target, widget const &object);
  void activate(size_t index);
  int hit(int x, int y) const;

public:
  menu(resources::data_store const &data);
  std::function<bool(widget &, int)> activated;
  std::function<void(render::framebuffer &, widget const &)> paint;
  widget &named(std::string_view global);
  std::span<widget> objects() { return widgets; }
  std::string_view current() const;
  void show(std::string_view global);
  void popup(std::vector<std::pair<std::string, int>> choices, std::function<void(int)> selected);
  bool dismiss_popup();
  void link_scroll(std::string_view text, std::string_view scrollbar);
  void bounds(std::string_view global, render::rectangle area);
  void back();
  void move(int x, int y);
  void button(bool down);
  void toggle_quick(bool visible);
  void press_named(std::string_view name);
  void render(render::framebuffer &target);
  bool quit_requested() const;
};
} // namespace apache::frontend
