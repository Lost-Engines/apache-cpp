find_package(OpenGL REQUIRED)
find_package(glfw3 3.4 CONFIG QUIET)
if(NOT TARGET glfw)
  include(FetchContent)
  set(GLFW_BUILD_DOCS OFF CACHE BOOL "Build GLFW documentation")
  set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "Build GLFW examples")
  set(GLFW_BUILD_TESTS OFF CACHE BOOL "Build GLFW tests")
  set(GLFW_INSTALL OFF CACHE BOOL "Install GLFW")
  FetchContent_Declare(glfw
    URL https://codeload.github.com/glfw/glfw/tar.gz/a74efa0d5628b74adc0426af4c5710e287fa7c2c
    URL_HASH SHA256=7e85e39ade1efdda00734d323a0230b5c938074e9fb2ddbe453bdbb281064d2c
  )
  FetchContent_MakeAvailable(glfw)
  if(APPLE)
    # GLFW's C files include Cocoa headers with Apple's blocks extension too.
    # Match Darker: compile these sources with Apple's Objective-C compiler.
    get_target_property(glfw_sources glfw SOURCES)
    foreach(source IN LISTS glfw_sources)
      if(source MATCHES "\\.c$")
        get_filename_component(source_path "${source}" ABSOLUTE BASE_DIR "${glfw_SOURCE_DIR}/src")
        set_source_files_properties("${source_path}" DIRECTORY "${glfw_SOURCE_DIR}/src" PROPERTIES LANGUAGE OBJC)
      endif()
    endforeach()
  endif()
endif()

