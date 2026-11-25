# ##############################################################################
# A Logan Thomas Barnes project
# ##############################################################################
cmake_minimum_required(VERSION 3.23)
project(LtbVlk VERSION 0.0.1)

# Prevent cyclic dependencies even for static libs
set(GLOBAL_DEPENDS_NO_CYCLES ON)

# Prevent cmake from creating build artifacts in the source directory
if (PROJECT_SOURCE_DIR STREQUAL PROJECT_BINARY_DIR)
  message(FATAL_ERROR "In-source builds are not allowed. Use a separate build directory (e.g. cmake -B build).")
endif ()

# ##############################################################################
# Options
# ##############################################################################
option(
  LTB_VLK_USE_STRICT_FLAGS
  "Use strict flags when building"
  OFF
)
option(
  LTB_VLK_BUILD_EXAMPLES
  "Build example applications"
  OFF
)
option(
  LTB_VLK_BUILD_TESTS
  "Build unit and GPU tests"
  OFF
)

# ##############################################################################
# CMake Package Manager
# ##############################################################################
include(cmake/CPM.cmake)

# ##############################################################################
# External Packages
# ##############################################################################
include(${CMAKE_CURRENT_LIST_DIR}/cmake/ThirdParty.cmake)

# ##############################################################################
# Configuration
# ##############################################################################
# A directory where generated files can be stored and referenced
set(LTB_VLK_GENERATED_DIR ${PROJECT_BINARY_DIR}/generated)

# Compiled SPIR-V shaders are written here (mirroring res/shaders).
set(LTB_VLK_SPIRV_DIR ${PROJECT_BINARY_DIR}/shaders)

configure_file(
  ${CMAKE_CURRENT_LIST_DIR}/src/ltb/ltb_vlk_config.hpp.in
  ${LTB_VLK_GENERATED_DIR}/ltb/vlk/ltb_vlk_config.hpp
)

add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/res/shaders)

# ##############################################################################
# Library
# ##############################################################################
file(
  GLOB_RECURSE
  LtbVlk_CORE_SOURCE_FILES
  LIST_DIRECTORIES
  false
  CONFIGURE_DEPENDS
  ${CMAKE_CURRENT_LIST_DIR}/src/ltb/cam/*.cpp
  ${CMAKE_CURRENT_LIST_DIR}/src/ltb/display/*.cpp
  ${CMAKE_CURRENT_LIST_DIR}/src/ltb/geom/*.cpp
  ${CMAKE_CURRENT_LIST_DIR}/src/ltb/gui/*.cpp
  ${CMAKE_CURRENT_LIST_DIR}/src/ltb/utils/*.cpp
  ${CMAKE_CURRENT_LIST_DIR}/src/ltb/vlk/*.cpp
  ${CMAKE_CURRENT_LIST_DIR}/src/ltb/window/*.cpp
)

# Inter-process communication (ipc::Channel) has a backend per platform.
if (CMAKE_SYSTEM_NAME STREQUAL "Linux")
  set(LtbVlk_IPC_PLATFORM linux) # Unix domain sockets that pass file descriptors
elseif (WIN32)
  set(LtbVlk_IPC_PLATFORM windows) # Named pipes that pass duplicated handles
endif ()

if (DEFINED LtbVlk_IPC_PLATFORM)
  file(
    GLOB_RECURSE
    LtbVlk_IPC_SOURCE_FILES
    LIST_DIRECTORIES
    false
    CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_LIST_DIR}/src/ltb/ipc/${LtbVlk_IPC_PLATFORM}/*.cpp
  )
  list(APPEND LtbVlk_CORE_SOURCE_FILES ${LtbVlk_IPC_SOURCE_FILES})
endif ()

add_library(
  LtbVlk
  ${LtbVlk_CORE_SOURCE_FILES}
)
add_library(
  LtbVlk::LtbVlk
  ALIAS
  LtbVlk
)
target_link_libraries(
  LtbVlk
  PUBLIC
  # Shaders
  LtbVlk::generate_spirv
  # Graphics
  Vulkan::Vulkan
  # Geometry
  glm::glm
  # Gui
  imgui::imgui
  # Utils
  Threads::Threads
  tl::expected
  magic_enum::magic_enum
  spdlog::spdlog
  cxxopts::cxxopts
  $<TARGET_NAME_IF_EXISTS:TBB::tbb>
)
target_include_directories(
  LtbVlk
  PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/include>
  $<BUILD_INTERFACE:${LTB_VLK_GENERATED_DIR}>
)
target_include_directories(
  LtbVlk
  SYSTEM
  PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/external/include>
)
target_compile_features(
  LtbVlk
  PUBLIC
  cxx_std_23
)
target_compile_definitions(
  LtbVlk
  PUBLIC
  # Public headers use std::min/std::max, which windows.h macros would break.
  $<$<PLATFORM_ID:Windows>:NOMINMAX>
  PRIVATE
  $<$<PLATFORM_ID:Windows>:_CRT_SECURE_NO_WARNINGS>
)
set_target_properties(
  LtbVlk
  PROPERTIES
  POSITION_INDEPENDENT_CODE ON
  WINDOWS_EXPORT_ALL_SYMBOLS ON
)
target_compile_options(
  LtbVlk
  PRIVATE
  # Ignore any headers using angle brackets on windows
  $<$<COMPILE_LANG_AND_ID:CXX,MSVC>:/experimental:external>
  $<$<COMPILE_LANG_AND_ID:CXX,MSVC>:/external:anglebrackets>
  $<$<COMPILE_LANG_AND_ID:CXX,MSVC>:/external:W0>
  # Hopefully temporary MSVC spdlog warning
  $<$<COMPILE_LANG_AND_ID:CXX,MSVC>:/wd4996>
)

# ##############################################################################
# Development Settings
# ##############################################################################
# Strict warnings for this project's own targets (library, apps, tests). Linked
# PRIVATE so projects that consume LtbVlk don't inherit -Werror and friends.
add_library(
  ltb_vlk_warnings
  INTERFACE
)
add_library(
  LtbVlk::warnings
  ALIAS
  ltb_vlk_warnings
)
if (LTB_VLK_USE_STRICT_FLAGS)
  target_compile_options(
    ltb_vlk_warnings
    INTERFACE
    # Strict warnings/errors with gcc and clang
    $<$<COMPILE_LANG_AND_ID:CXX,GNU,Clang,AppleClang>:-Wall>
    $<$<COMPILE_LANG_AND_ID:CXX,GNU,Clang,AppleClang>:-Wextra>
    $<$<COMPILE_LANG_AND_ID:CXX,GNU,Clang,AppleClang>:-Werror>
    $<$<COMPILE_LANG_AND_ID:CXX,GNU,Clang,AppleClang>:-Wpedantic>
    $<$<COMPILE_LANG_AND_ID:CXX,GNU,Clang,AppleClang>:-Wunused>
    $<$<COMPILE_LANG_AND_ID:CXX,GNU,Clang,AppleClang>:-pedantic-errors>
    $<$<COMPILE_LANG_AND_ID:CXX,GNU,Clang,AppleClang>:-Winit-self>
    $<$<COMPILE_LANG_AND_ID:CXX,GNU,Clang,AppleClang>:-Wold-style-cast>
    $<$<COMPILE_LANG_AND_ID:CXX,GNU,Clang,AppleClang>:-Woverloaded-virtual>
    $<$<COMPILE_LANG_AND_ID:CXX,GNU,Clang,AppleClang>:-Wsign-conversion>
    $<$<COMPILE_LANG_AND_ID:CXX,GNU,Clang,AppleClang>:-Wshadow>
    $<$<COMPILE_LANG_AND_ID:CXX,GNU,Clang,AppleClang>:-Wmissing-declarations>
    $<$<COMPILE_LANG_AND_ID:CXX,GNU,Clang,AppleClang>:-Wmissing-include-dirs>
    $<$<COMPILE_LANG_AND_ID:CXX,GNU,Clang,AppleClang>:-Wno-unknown-pragmas>
    $<$<COMPILE_LANG_AND_ID:CXX,GNU>:-Wformat-truncation>
    $<$<COMPILE_LANG_AND_ID:CXX,Clang,AppleClang>:-Wimplicit-float-conversion>
    $<$<COMPILE_LANG_AND_ID:CXX,GNU>:-Wfloat-conversion>
    $<$<COMPILE_LANG_AND_ID:CXX,GNU,Clang,AppleClang>:-fstrict-aliasing>
    $<$<COMPILE_LANG_AND_ID:CXX,GNU,Clang,AppleClang>:-Wstrict-aliasing>
    $<$<COMPILE_LANG_AND_ID:CXX,Clang,AppleClang>:-Wcast-align>
    $<$<COMPILE_LANG_AND_ID:CXX,GNU>:-Wcast-align=strict>
    # Strict warnings/errors with msvc
    $<$<COMPILE_LANG_AND_ID:CXX,MSVC>:/W4>
    $<$<COMPILE_LANG_AND_ID:CXX,MSVC>:/WX>
  )
endif ()
target_link_libraries(
  LtbVlk
  PRIVATE
  $<BUILD_INTERFACE:LtbVlk::warnings>
)

# ##############################################################################
# Applications
# ##############################################################################
if (LTB_VLK_BUILD_EXAMPLES)

  # OpenGL is only needed by the `raw` example, which is skipped if it's missing.
  find_package(OpenGL)

  if (OpenGL_FOUND)
    # Library allowing OpenGL functions to be loaded at runtime.
    add_library(
      ltb_ogl_loader
      ${CMAKE_CURRENT_LIST_DIR}/external/src/GL/gl3w.c
    )
    add_library(
      ltb_ogl::loader
      ALIAS
      ltb_ogl_loader
    )
    target_link_libraries(
      ltb_ogl_loader
      PUBLIC
      OpenGL::GL
    )
    target_include_directories(
      ltb_ogl_loader
      SYSTEM
      PUBLIC
      $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/external/include>
    )
  endif ()

  add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/src/ltb/apps)
endif ()

# ##############################################################################
# Tests
# ##############################################################################
if (LTB_VLK_BUILD_TESTS)
  enable_testing()
  add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/tests)
endif ()
