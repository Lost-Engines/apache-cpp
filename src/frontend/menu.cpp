#include "menu.h"
#include <algorithm>
#include <stdexcept>

namespace apache::frontend {
menu::menu(resources::data_store const &data) : data{data}, font{resources::read_iff(data.read("FONTS/11X15.LBM"))} {
  for(size_t i{0}; i != widgets.size(); ++i) for(auto const &name : widgets[i].globals) globals[name] = i;
  clips.resize(widgets.size());
  show("_main_screen");
}
widget &menu::named(std::string_view const global) {
  auto const found{globals.find(global)};
  if(found == globals.end()) throw std::runtime_error{"Unknown menu widget: " + std::string{global}};
  return widgets[found->second];
}
std::string_view menu::current() const {
  return screens.empty() ? std::string_view{} : widgets[screens.back()].globals.front();
}
resources::indexed_image const &menu::image(std::string const &name) {
  auto found{images.find(name)};
  if(found == images.end()) found = images.emplace(name, resources::read_iff(data.read(name))).first;
  return found->second;
}
void menu::link_scroll(std::string_view const text, std::string_view const scrollbar) {
  scroll_links.emplace_back(globals.at(std::string{text}), globals.at(std::string{scrollbar}));
}
void menu::scroll_changed(size_t const index) {
  for(auto const &[text, bar] : scroll_links) if(bar == index) widgets[text].start_line = static_cast<unsigned int>(std::max(0, widgets[bar].range[2]));
}
void menu::bounds(std::string_view const global, render::rectangle const area) {
  auto const index{globals.at(std::string{global})};
  auto const old{widgets[index].area};
  int const dx{area.left - old.left}, dy{area.top - old.top};
  for(size_t i{index + 1}; i < widgets.size(); ++i) {
    int parent{widgets[i].parent};
    while(parent >= 0 && static_cast<size_t>(parent) != index) parent = widgets[static_cast<size_t>(parent)].parent;
    if(parent >= 0) { auto &r{widgets[i].area}; r.left += dx; r.right += dx; r.top += dy; r.bottom += dy; }
  }
  widgets[index].area = area;
}
void menu::show(std::string_view const global) {
  active_popup.reset();
  auto const found{globals.find(global)};
  if(found == globals.end()) throw std::runtime_error{"Unknown menu screen: " + std::string{global}};
  auto const index{found->second};
  auto const stacked{std::ranges::find(screens, index)};
  if(stacked != screens.end()) screens.erase(stacked, screens.end());
  screens.push_back(index);
  for(auto &object : widgets) if(object.parent == 0) object.flags &= ~1u;
  widgets[index].flags |= 1;
  hovered = pressed = -1;
}
void menu::back() {
  if(dismiss_popup()) return;
  if(!screens.empty() && screens.back() == globals.at("_main_screen") && (widgets[globals.at("_quick_start_options")].flags & 1)) {
    toggle_quick(false);
    return;
  }
  if(screens.size() <= 1) {
    show("_exit_screen");
    return;
  }
  screens.pop_back();
  auto const index{screens.back()};
  for(auto &object : widgets) if(object.parent == 0) object.flags &= ~1u;
  widgets[index].flags |= 1;
  hovered = pressed = -1;
}
void menu::popup(std::vector<std::pair<std::string, int>> choices, std::function<void(int)> selected) {
  if(choices.empty()) return;
  int width{0}; for(auto const &choice : choices) width = std::max(width, font.width(choice.first) + 8);
  int const height{static_cast<int>(choices.size()) * 19 + 3};
  int const x{std::clamp(cursor_x - width / 2, 0, std::max(0, 639 - width))};
  int const y{std::clamp(cursor_y - height / 2, 0, std::max(0, 479 - height))};
  active_popup = popup_state{std::move(choices), std::move(selected), {x, y, x + width, y + height}};
  move(cursor_x, cursor_y);
}
bool menu::dismiss_popup() { if(!active_popup) return false; active_popup.reset(); return true; }
void menu::toggle_quick(bool const visible) {
  auto &object{widgets.at(globals.at("_quick_start_options"))};
  object.flags = (object.flags & ~1u) | static_cast<unsigned int>(visible);
}
void menu::press_named(std::string_view const name) {
  for(auto &object : widgets) if(object.type == 9 || object.type == 10) object.on = object.name == name;
}
void menu::draw_widget(render::framebuffer &target, widget const &object) {
  auto const &r{object.area};
  bool const button{object.type == 3 || object.type == 9 || object.type == 10};
  bool const clear{(object.flags & 0x4000) != 0};
  if(object.type == 16 || (button && !object.picture.empty() && object.on)) {
    if(!object.picture.empty()) {
      auto const &picture{image(object.picture)};
      target.blit(picture, r.left, r.top);
      if(object.type == 16) {
        unsigned int start{0}, count{246};
        switch(object.palette_part) {
        case 1: start = 168; count = 78; break;
        case 2: count = 64; break;
        case 3: start = 64; count = 182; break;
        case 4: count = 0; break;
        case 5: count = 192; break;
        case 6: count = 112; break;
        case 7: start = 112; count = 134; break;
        default: break;
        }
        std::copy_n(picture.colours.begin() + start, count, target.image.colours.begin() + start);
      }
    } else target.fill(r, 0);
  }
  if(object.type == 5) target.fill(r, 0);
  if(object.type == 0 && !clear) target.fill(r, object.colour);
  if(object.type == 2 && !clear) {
    target.fill(r, object.colour);
    target.surround(r, object.on);
  }
  if(button) {
    if(object.picture.empty()) {
      if(!clear) target.fill(r, object.type == 9 ? (object.on ? 247 : 249) : object.colour);
      if(!(object.flags & 0x400000)) target.surround(r, object.on);
    }
    if(!object.text.empty()) {
      int const x{(r.left + r.right) / 2 - font.width(object.text) / 2};
      int const y{(r.top + r.bottom) / 2 - (static_cast<int>(object.font_height + 1) / 2 - 1) - (object.picture.empty() ? 0 : 3)};
      if(object.shadow) font.draw(target, object.text, r.left + 1, r.top + 1, 0, static_cast<int>(object.font_height + 1));
      font.draw(target, object.text, object.shadow ? r.left : x, object.shadow ? r.top : y, object.text_colour, static_cast<int>(object.font_height + 1));
    }
    if(object.flags & 0x10000) {
      for(int row{0}; row != 4; ++row) {
        target.fill({r.right - 9 - row, r.top + 3 + row, r.right - 9 + row, r.top + 3 + row}, object.text_colour);
        target.fill({r.right - 12 + row, r.top + 9 + row, r.right - 6 - row, r.top + 9 + row}, object.text_colour);
      }
    }
  }
  if(object.type == 12 && object.font_height) {
    int const padding{(object.flags & 0x20000) ? 3 : 0};
    auto const lines{font.lines(object.text, r.right - r.left - padding, (object.flags & 0x100000) != 0)};
    int y{r.top};
    for(size_t i{object.start_line}; i < lines.size() && y <= r.bottom; ++i, y += static_cast<int>(object.font_height + 1)) {
      if((object.flags & 0x10) && static_cast<int>(i) == object.selected_line) target.fill({r.left, y, r.right, y + static_cast<int>(object.font_height) - 4}, 248);
      if(object.shadow) font.draw(target, lines[i], r.left + padding + 1, y + 1, 0);
      font.draw(target, lines[i], r.left + padding, y, object.text_colour);
    }
  }
  if(object.type == 13) {
    if(!clear) target.fill(r, 249);
    target.surround(r);
    int const length{r.bottom - r.top - 4};
    int const denominator{std::max(1, object.range[1] - object.range[0] + 1)};
    int top{r.top + 2 + (object.range[2] - object.range[0]) * length / denominator};
    int bottom{top + std::max(12, object.range[3] * length / denominator)};
    if(bottom > r.bottom - 2) { bottom = r.bottom - 2; if(top + 12 > bottom) top = r.bottom - 14; }
    target.surround({r.left + 2, top, r.right - 2, bottom}, object.on);
  }
  if(object.type == 14) {
    if(!clear) target.fill(r, 249);
    target.surround(r);
    int const length{r.right - r.left - 4};
    int const denominator{std::max(1, object.range[1] - object.range[0] + 1)};
    int left{r.left + 2 + (object.range[2] - object.range[0]) * length / denominator};
    int right{left + std::max(12, object.range[3] * length / denominator)};
    if(right > r.right - 2) {
      right = r.right - 2;
      if(left + 12 > right) left = r.right - 14;
    }
    for(int i{1}; i != 10; ++i) {
      int const x{r.left + 2 + length / 10 * i};
      target.point(x, (r.top + r.bottom) / 2, 251);
      target.point(x, (r.top + r.bottom) / 2 + 1, 251);
    }
    target.surround({left, r.top + 2, right, r.bottom - 2}, object.on);
  }
  if(object.type == 17 && !object.sprite.empty()) {
    auto found{sprites.find(object.sprite)};
    if(found == sprites.end()) {
      auto const &source{image(object.sprite)};
      auto const &g{object.sprite_geometry};
      render::framebuffer sprite{static_cast<unsigned int>(g[0]), static_cast<unsigned int>(g[1])};
      sprite.clip = {0, 0, g[4] - 1, g[5] - 1};
      sprite.blit(source, 0, 0);
      found = sprites.emplace(object.sprite, std::move(sprite.image)).first;
    }
    target.blit(found->second, r.left - object.sprite_geometry[2], r.top - object.sprite_geometry[3], true);
  }
}
void menu::render(render::framebuffer &target) {
  for(auto const &[text, bar] : scroll_links) {
    auto const &object{widgets[text]};
    auto const count{font.lines(object.text, object.area.right - object.area.left - ((object.flags & 0x20000) ? 3 : 0), (object.flags & 0x100000) != 0).size()};
    widgets[bar].range = {0, std::max(0, static_cast<int>(count) - 1), static_cast<int>(object.start_line), (object.area.bottom - object.area.top) / static_cast<int>(object.font_height + 1)};
  }
  target.reset_clip();
  auto const screen_clip{target.clip};
  target.clear();
  for(size_t i{0}; i != widgets.size(); ++i) {
    auto const &object{widgets[i]};
    clips[i] = {};
    if(!(object.flags & 1)) continue;
    auto const parent_clip{object.parent < 0 ? screen_clip : clips[static_cast<size_t>(object.parent)]};
    clips[i] = render::intersect(parent_clip, object.area);
    if(clips[i].left > clips[i].right || clips[i].top > clips[i].bottom) continue;
    target.clip = clips[i];
    draw_widget(target, object);
    if(paint) paint(target, object);
  }
  // Original UI palette, shared by every menu and independent of the backdrop.
  resources::palette &palette{target.image.colours};
  std::array<resources::rgb, 7> constexpr colours{{{66, 97, 100}, {76, 112, 116}, {135, 169, 171}, {102, 151, 154}, {178, 203, 204}, {255, 255, 255}, {0, 127, 127}}};
  std::ranges::copy(colours, palette.begin() + 246);
  target.reset_clip();
  if(active_popup) {
    auto const &popup{*active_popup}; auto const &r{popup.area};
    target.fill(r, 249); target.surround(r);
    for(size_t i{0}; i != popup.choices.size(); ++i) {
      int const y{r.top + 2 + static_cast<int>(i) * 19};
      if(static_cast<int>(i) == popup.hovered) target.surround({r.left + 2, y, r.right - 2, y + 18});
      font.draw(target, popup.choices[i].first, r.left + 4, y + 2, 251);
    }
  }
}
int menu::hit(int const x, int const y) const {
  for(size_t i{widgets.size()}; i-- > 0;) if((widgets[i].flags & 0x8000) && clips[i].contains(x, y)) return static_cast<int>(i);
  return -1;
}
void menu::move(int const x, int const y) {
  cursor_x = x; cursor_y = y;
  if(active_popup) {
    auto &p{*active_popup};
    p.hovered = p.area.contains(x, y) && y >= p.area.top + 2 && y < p.area.bottom - 1 ? (y - p.area.top - 2) / 19 : -1;
    return;
  }
  if(pressed >= 0 && widgets[static_cast<size_t>(pressed)].type == 14 && scroll_direction == 0) {
    auto &bar{widgets[static_cast<size_t>(pressed)]};
    bar.range[2] = std::clamp(drag_position + x - drag_y, bar.range[0], bar.range[1]);
    if(activated) activated(bar, bar.range[2]);
  }
  if(pressed >= 0 && widgets[static_cast<size_t>(pressed)].type == 13 && scroll_direction == 0) {
    auto &bar{widgets[static_cast<size_t>(pressed)]};
    bar.range[2] = std::clamp(drag_position + (y - drag_y) * (bar.range[1] - bar.range[0] + 1) / std::max(1, bar.area.bottom - bar.area.top - 4), bar.range[0], bar.range[1]);
    scroll_changed(static_cast<size_t>(pressed));
  }
  hovered = hit(x, y);
  for(size_t i{0}; i != widgets.size(); ++i) if(widgets[i].type == 9 && !widgets[i].picture.empty()) widgets[i].on = static_cast<int>(i) == hovered;
}
void menu::button(bool const down) {
  if(active_popup) {
    if(!down) { auto selected{std::move(active_popup->selected)}; auto const line{active_popup->hovered}; auto choices{std::move(active_popup->choices)}; active_popup.reset(); if(line >= 0 && static_cast<size_t>(line) < choices.size()) selected(choices[static_cast<size_t>(line)].second); }
    return;
  }
  if(down) {
    pressed = hovered;
    if(pressed >= 0 && widgets[static_cast<size_t>(pressed)].type == 9) widgets[static_cast<size_t>(pressed)].on = true;
    if(pressed >= 0 && (widgets[static_cast<size_t>(pressed)].type == 13 || widgets[static_cast<size_t>(pressed)].type == 14)) {
      auto &bar{widgets[static_cast<size_t>(pressed)]}; auto const &r{bar.area};
      bool const horizontal{bar.type == 14};
      int const start{horizontal ? r.left : r.top}, end{horizontal ? r.right : r.bottom}, cursor{horizontal ? cursor_x : cursor_y};
      int const length{end - start - 4}, denominator{std::max(1, bar.range[1] - bar.range[0] + 1)};
      int top{start + 2 + (bar.range[2] - bar.range[0]) * length / denominator};
      int bottom{top + std::max(12, bar.range[3] * length / denominator)};
      if(bottom > end - 2) { bottom = end - 2; if(top + 12 > bottom) top = end - 14; }
      scroll_direction = cursor < top ? -1 : cursor > bottom ? 1 : 0;
      drag_y = cursor; drag_position = bar.range[2]; bar.on = scroll_direction == 0;
    }
  } else {
    if(pressed >= 0 && (widgets[static_cast<size_t>(pressed)].type == 13 || widgets[static_cast<size_t>(pressed)].type == 14)) {
      auto &bar{widgets[static_cast<size_t>(pressed)]};
      if(pressed == hovered && scroll_direction) bar.range[2] = std::clamp(bar.range[2] + scroll_direction * bar.range[3], bar.range[0], bar.range[1]);
      bar.on = false; scroll_changed(static_cast<size_t>(pressed));
      if(bar.type == 14 && activated) activated(bar, bar.range[2]);
    } else if(pressed >= 0) {
      auto &object{widgets[static_cast<size_t>(pressed)]};
      if(object.type == 9 && object.picture.empty()) object.on = false;
      if(pressed == hovered) activate(static_cast<size_t>(pressed));
    }
    pressed = -1;
  }
}
void menu::activate(size_t const index) {
  auto &object{widgets[index]};
  if(object.type == 4 && object.parent >= 0) { widgets[static_cast<size_t>(object.parent)].flags &= ~1u; return; }
  int const line{(cursor_y - object.area.top) / static_cast<int>(object.font_height + 1) + static_cast<int>(object.start_line)};
  if(activated && activated(object, line)) return;
  auto const action{object.action};
  if(action == "notify_quit_game_button_") quitting = true;
  else if(action == "notify_cancel_quit_game_" || action == "notify_exit_button_" || action == "notify_preferences_exit_") back();
  else if(action == "notify_quick_start_button_") toggle_quick(true);
  else if(action == "notify_flight_button_") show("_flight_screen");
  else if(action == "notify_preferences_button_") show("_preferences_screen");
  else if(action == "notify_log_button_") show("_log_screen");
  else if(action == "notify_quick_start_topten_button_") show("_top_ten_screen");
  else if(action == "notify_quick_start_arcade_button_" || action == "notify_quick_start_invin_button_") show("_world_screen");
}
bool menu::quit_requested() const {
  return quitting;
}
} // namespace apache::frontend
