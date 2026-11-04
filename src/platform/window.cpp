#include "window.h"
#include <algorithm>
#include <cmath>
#include <iostream>
#include <stdexcept>
#ifdef _WIN32
#include <GL/glext.h>
#endif // _WIN32

namespace apache::platform {
namespace {
struct pair { int x; int y; };
struct display_area { pair position; pair size; };
display_area viewport(int const width, int const height, int const logical_width, int const logical_height) {
  double const available{std::min(width / static_cast<double>(logical_width),height / static_cast<double>(logical_height))};
  double const fit{available >= 1 ? std::floor(available) : available};
  pair const fitted{std::max(1,static_cast<int>(logical_width * fit)),std::max(1,static_cast<int>(logical_height * fit))};
  return {
    .position{(width - fitted.x) / 2,(height - fitted.y) / 2},
    .size{pair{fitted}},
  };
}
} // anonymous namespace


glfw_runtime::glfw_runtime() {
  glfwSetErrorCallback([](int const code, char const *const message){ std::cerr << "ERROR: GLFW " << code << ": " << message << std::endl; });
  // Hat directions are handled separately from the four original fire buttons.
  glfwInitHint(GLFW_JOYSTICK_HAT_BUTTONS, GLFW_FALSE);
  if(!glfwInit()) throw std::runtime_error{"GLFW could not initialise a desktop display"};
}
glfw_runtime::~glfw_runtime() {
  glfwTerminate();
}

window::window(unsigned int const scale) : handle{nullptr, glfwDestroyWindow} {
  /// Match Darker's floating-window sizing and nearest-neighbour presentation; only the final CPU image uses OpenGL
  bool const automatic{scale == 0}, compositor_sizing{automatic && glfwGetPlatform() == GLFW_PLATFORM_WAYLAND};
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR,2);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR,1);
  glfwWindowHint(GLFW_VISIBLE,GLFW_FALSE);
  glfwWindowHint(GLFW_MAXIMIZED,compositor_sizing ? GLFW_TRUE : GLFW_FALSE);
  auto const initial_scale{static_cast<int>(scale ? scale : 2)};
  handle.reset(glfwCreateWindow(logical_width * initial_scale,logical_height * initial_scale,"Apache C++",nullptr,nullptr));
  if(!handle) throw std::runtime_error{"cannot create the Apache window"};
  if(automatic && !compositor_sizing) {
    if(auto *monitor{glfwGetPrimaryMonitor()}) {
      int x{0}, y{0}, width{0}, height{0}, left{0}, top{0}, right{0}, bottom{0};
      glfwGetMonitorWorkarea(monitor,&x,&y,&width,&height);
      glfwGetWindowFrameSize(handle.get(),&left,&top,&right,&bottom);
      int const fit{std::max(1,std::min((width - left - right) / logical_width,(height - top - bottom) / logical_height))};
      glfwSetWindowSize(handle.get(),logical_width * fit,logical_height * fit);
      glfwSetWindowPos(handle.get(),x + left + (width - left - right - logical_width * fit) / 2,y + top + (height - top - bottom - logical_height * fit) / 2);
    }
  }
  glfwShowWindow(handle.get());
  if(compositor_sizing) {
    int width{0}, height{0};
    glfwGetWindowSize(handle.get(),&width,&height);
    glfwRestoreWindow(handle.get());
    auto const deadline{glfwGetTime() + 1};
    while(glfwGetWindowAttrib(handle.get(),GLFW_MAXIMIZED) && glfwGetTime() < deadline) glfwWaitEventsTimeout(.01);
    int const fit{std::max(1,std::min(width / logical_width,height / logical_height))};
    glfwSetWindowSize(handle.get(),logical_width * fit,logical_height * fit);
  }
  glfwMakeContextCurrent(handle.get());
  glfwSwapInterval(1);
  glGenTextures(1,&texture);
  glBindTexture(GL_TEXTURE_2D,texture);
  glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);
  glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA8,logical_width,logical_height,0,GL_RGBA,GL_UNSIGNED_BYTE,nullptr);
  if(glGetError() != GL_NO_ERROR) throw std::runtime_error{"cannot allocate the Apache display texture"};
  glDisable(GL_DITHER);
  glEnable(GL_TEXTURE_2D);
  glTexEnvi(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_REPLACE);
  glfwSetWindowFocusCallback(handle.get(),[](GLFWwindow *const target, int const focused){
    if(!focused) glfwSetInputMode(target,GLFW_CURSOR,GLFW_CURSOR_NORMAL);
  });
}

window::~window() {
  glDeleteTextures(1,&texture);
}
GLFWwindow *window::get() const {
  return handle.get();
}
bool window::focused() const {
  return glfwGetWindowAttrib(handle.get(),GLFW_FOCUSED) == GLFW_TRUE;
}
bool window::poll() {
  glfwPollEvents();
  return !glfwWindowShouldClose(handle.get());
}
std::optional<cursor> window::cursor_position() const {
  int width{0}, height{0}, framebuffer_width{0}, framebuffer_height{0};
  glfwGetWindowSize(handle.get(), &width, &height);
  glfwGetFramebufferSize(handle.get(), &framebuffer_width, &framebuffer_height);
  if(width <= 0 || height <= 0 || framebuffer_width <= 0 || framebuffer_height <= 0) return {};
  double x{0}, y{0};
  glfwGetCursorPos(handle.get(), &x, &y);
  auto const area{viewport(framebuffer_width, framebuffer_height, logical_width, logical_height)};
  x = (x * framebuffer_width / width - area.position.x) * logical_width / area.size.x;
  y = (y * framebuffer_height / height - (framebuffer_height - area.position.y - area.size.y)) * logical_height / area.size.y;
  if(x < 0 || y < 0 || x >= logical_width || y >= logical_height) return {};
  return cursor{x, y};
}
void window::title(std::string const &text) {
  glfwSetWindowTitle(handle.get(),text.c_str());
}
void window::present(render::framebuffer const &frame) {
  int width{0}, height{0};
  glfwGetFramebufferSize(handle.get(),&width,&height);
  if(width <= 0 || height <= 0) return;
  if(logical_width != static_cast<int>(frame.image.width) || logical_height != static_cast<int>(frame.image.height)) {
    logical_width = static_cast<int>(frame.image.width);
    logical_height = static_cast<int>(frame.image.height);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, logical_width, logical_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
  }
  auto const pixels{frame.colour_pixels()};
  auto const area{viewport(width, height, logical_width, logical_height)};
  glViewport(0,0,width,height);
  glClearColor(0,0,0,1);
  glClear(GL_COLOR_BUFFER_BIT);
  glViewport(area.position.x,area.position.y,area.size.x,area.size.y);
  glBindTexture(GL_TEXTURE_2D,texture);
  glTexSubImage2D(GL_TEXTURE_2D,0,0,0,logical_width,logical_height,GL_RGBA,GL_UNSIGNED_BYTE,pixels.data());
  glBegin(GL_TRIANGLE_STRIP);
  glTexCoord2f(0,0); glVertex2f(-1,1);
  glTexCoord2f(0,1); glVertex2f(-1,-1);
  glTexCoord2f(1,0); glVertex2f(1,1);
  glTexCoord2f(1,1); glVertex2f(1,-1);
  glEnd();
  glfwSwapBuffers(handle.get());
}

} // namespace apache::platform
