#include <charconv>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>
#include "frontend/menu.h"
#include "platform/window.h"
#include "resources/data_store.h"

namespace {
unsigned int number(std::string_view const text) {
  unsigned int result{0};
  auto const [end, error]{std::from_chars(text.data(), text.data() + text.size(), result)};
  if(error != std::errc{} || end != text.data() + text.size()) throw std::runtime_error{"Expected a non-negative integer"};
  return result;
}
struct events {
  std::vector<int> keys;
  std::vector<bool> clicks;
};
} // namespace

int main(int const argc, char const *const argv[]) try {
  std::filesystem::path directory{"data"};
  std::string selected_menu{"_main_screen"};
  std::string capture;
  std::string pressed;
  unsigned int scale{0}, frame_limit{0};
  bool quick{false};
  for(int i{1}; i < argc; ++i) {
    std::string_view const key{argv[i]};
    auto const value{[&]()->std::string_view{
      if(++i == argc) throw std::runtime_error{"Missing value for " + std::string{key}};
      return argv[i];
    }};
    if(key == "--help") {
      std::cout << "Apache C++ reconstruction\n"
                   "Usage: apache [--data-dir DIRECTORY] [--scale N]\n"
                   "  --data-dir DIRECTORY  Original installation or extracted CD contents\n"
                   "  --scale N             Integer scale, or 0 for automatic sizing\n"
                   "Development checks:\n"
                   "  --menu NAME           Select an original frontend screen\n"
                   "  --capture FILE.ppm    Render one menu frame without opening a window\n"
                   "  --press NAME          Show a pressed menu button\n"
                   "  --quick               Show the Quickstart choices\n"
                   "  --frames N            Close after N window frames\n";
      return EXIT_SUCCESS;
    }
    if(key == "--data-dir") directory = value();
    else if(key == "--scale") {
      scale = number(value());
      if(scale > 16) throw std::runtime_error{"Window scale must be between 0 and 16"};
    } else if(key == "--frames") frame_limit = number(value());
    else if(key == "--menu") selected_menu = "_" + std::string{value()} + "_screen";
    else if(key == "--capture") capture = value();
    else if(key == "--press") pressed = value();
    else if(key == "--quick") quick = true;
    else throw std::runtime_error{"Unknown option: " + std::string{key}};
  }
  apache::resources::data_store const data{directory};
  apache::frontend::menu menu{data};
  menu.show(selected_menu);
  menu.toggle_quick(quick);
  if(!pressed.empty()) menu.press_named(pressed);
  apache::render::framebuffer frame{640, 480};
  menu.render(frame);
  if(!capture.empty()) {
    apache::resources::write_ppm(capture, frame.image);
    return EXIT_SUCCESS;
  }
  apache::platform::window window{scale};
  events input;
  glfwSetWindowUserPointer(window.get(), &input);
  glfwSetKeyCallback(window.get(), [](GLFWwindow *const handle, int const key, int, int const action, int){
    if(action == GLFW_PRESS) static_cast<events*>(glfwGetWindowUserPointer(handle))->keys.push_back(key);
  });
  glfwSetMouseButtonCallback(window.get(), [](GLFWwindow *const handle, int const button, int const action, int){
    if(button == GLFW_MOUSE_BUTTON_LEFT) static_cast<events*>(glfwGetWindowUserPointer(handle))->clicks.push_back(action == GLFW_PRESS);
  });
  unsigned int frames{0};
  while(window.poll() && !menu.quit_requested()) {
    auto const cursor{window.cursor_position()};
    menu.move(cursor ? static_cast<int>(cursor->x) : -1, cursor ? static_cast<int>(cursor->y) : -1);
    for(bool const click : input.clicks) menu.button(click);
    input.clicks.clear();
    for(int const key : input.keys) {
      if(key == GLFW_KEY_ESCAPE) menu.back();
      else if(key == GLFW_KEY_F1) menu.show("_help_screen");
    }
    input.keys.clear();
    menu.render(frame);
    window.present(frame);
    if(frame_limit && ++frames == frame_limit) break;
  }
  return EXIT_SUCCESS;
} catch(std::exception const &error) {
  std::cerr << "ERROR: " << error.what() << std::endl;
  return EXIT_FAILURE;
}
