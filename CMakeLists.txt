cmake_minimum_required(VERSION 3.30)
project(apache VERSION 0.1.0 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_SCAN_FOR_MODULES OFF)
include(CTest)

add_library(apache_core
  src/math/geometry.cpp
  src/math/geometry.h
  src/math/fixed.h
  src/math/trigonometry.cpp
  src/math/trigonometry.h
  src/resources/features.cpp
  src/resources/features.h
  src/render/sector_scan.cpp
  src/render/sector_scan.h
  src/render/feature_renderer.cpp
  src/render/feature_renderer.h
  src/resources/model_metadata.cpp
  src/resources/missions.cpp
  src/resources/missions.h
  src/resources/models.cpp
  src/resources/models.h
  src/frontend/menu.cpp
  src/frontend/menu.h
  src/frontend/widget.cpp
  src/frontend/widget.h
  src/render/terrain_renderer.cpp
  src/render/terrain_renderer.h
  src/render/terrain_shading.cpp
  src/render/terrain_shading.h
  src/resources/terrain_textures.cpp
  src/resources/terrain_textures.h
  src/render/terrain_projection.cpp
  src/render/terrain_projection.h
  src/render/draw_queue.cpp
  src/render/draw_queue.h
  src/render/mobile_scene.cpp
  src/render/mobile_scene.h
  src/render/shadows.cpp
  src/render/shadows.h
  src/render/model_renderer.cpp
  src/render/model_renderer.h
  src/render/avionics.cpp
  src/render/avionics.h
  src/render/helmet_display.cpp
  src/render/helmet_display.h
  src/render/mfd_flight_plan.cpp
  src/render/mfd_radar.cpp
  src/render/mfd_weapons.cpp
  src/render/map_renderer.cpp
  src/render/map_renderer.h
  src/render/mfd.cpp
  src/render/mfd.h
  src/render/panel_instruments.cpp
  src/render/panel_instruments.h
  src/render/cockpit.cpp
  src/render/cockpit.h
  src/render/instrument_font.cpp
  src/render/instrument_font.h
  src/render/palette_effects.cpp
  src/render/palette_effects.h
  src/resources/executable.cpp
  src/resources/executable.h
  src/render/scene_palette.cpp
  src/render/scene_palette.h
  src/simulation/arcade_flight.cpp
  src/simulation/arcade_flight.h
  src/simulation/realistic_flight.cpp
  src/simulation/realistic_flight.h
  src/simulation/rotor_dynamics.cpp
  src/simulation/rotor_dynamics.h
  src/simulation/flight_systems.cpp
  src/simulation/flight_systems.h
  src/simulation/instrument_state.cpp
  src/simulation/instrument_state.h
  src/simulation/mfd_state.cpp
  src/simulation/mfd_state.h
  src/simulation/fragments.cpp
  src/simulation/fragments.h
  src/render/fragment_renderer.cpp
  src/render/fragment_renderer.h
  src/simulation/craters.cpp
  src/simulation/craters.h
  src/render/crater_renderer.cpp
  src/render/crater_renderer.h
  src/simulation/effects.cpp
  src/simulation/effects.h
  src/render/effect_renderer.cpp
  src/render/effect_renderer.h
  src/render/clouds.cpp
  src/render/clouds.h
  src/render/celestial.cpp
  src/render/celestial.h
  src/render/landscape.cpp
  src/render/landscape.h
  src/render/horizon.cpp
  src/render/horizon.h
  src/render/projection2.cpp
  src/render/projection2.h
  src/render/projection.cpp
  src/render/projection.h
  src/render/perspective_rasterizer.cpp
  src/render/rasterizer.cpp
  src/render/rasterizer.h
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
  src/resources/texture.cpp
  src/resources/texture.h
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

add_executable(apache_inspect src/inspect.cpp src/inspect_flight.cpp src/inspect_flight.h src/inspect_arcade.cpp src/inspect_arcade.h)
target_link_libraries(apache_inspect PRIVATE apache_core)

if(BUILD_TESTING)
  add_executable(resource_tests tests/resources.cpp)
  target_link_libraries(resource_tests PRIVATE apache_core)
  add_test(NAME resource_tests COMMAND resource_tests)
endif()
