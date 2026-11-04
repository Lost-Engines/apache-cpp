cmake_minimum_required(VERSION 3.30)
project(apache VERSION 0.1.0 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_SCAN_FOR_MODULES OFF)
include(CTest)

add_library(apache_core
  src/frontend/menu.cpp
  src/frontend/menu.h
  src/frontend/widget.cpp
  src/frontend/widget.h
  src/render/framebuffer.cpp
  src/render/framebuffer.h
  src/render/menu_font.cpp
  src/render/menu_font.h
  src/resources/reader.cpp
  src/resources/reader.h
  src/resources/data_store.cpp
  src/resources/data_store.h
  src/resources/image.cpp
  src/resources/image.h
  src/resources/terrain.cpp
  src/resources/terrain.h
  src/configuration/preferences.cpp
  src/configuration/preferences.h
)
target_include_directories(apache_core PUBLIC src)
target_compile_options(apache_core PRIVATE
  $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Wconversion;-ffp-contract=off>
)

include(cmake/dependencies.cmake)
add_executable(apache
  src/main.cpp
  src/platform/window.cpp
  src/platform/window.h
)
target_link_libraries(apache PRIVATE apache_core glfw OpenGL::GL)
target_compile_options(apache PRIVATE $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Wconversion>)

add_executable(apache_inspect src/inspect.cpp)
target_link_libraries(apache_inspect PRIVATE apache_core)

if(BUILD_TESTING)
  add_executable(resource_tests tests/resources.cpp)
  target_link_libraries(resource_tests PRIVATE apache_core)
  add_test(NAME resource_tests COMMAND resource_tests)
endif()
