# ##############################################################################
# A Logan Thomas Barnes project
# ##############################################################################
cmake_minimum_required(VERSION 3.23)
project(LtbVlk VERSION 0.0.1)

# Prevent cyclic dependencies even for static libs
set(GLOBAL_DEPENDS_NO_CYCLES ON)

# Prevent cmake from creating build artifacts in the source directory
set(CMAKE_DISABLE_IN_SOURCE_BUILD ON)

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
set(LTB_VLK_GENERATED_DIR ${CMAKE_BINARY_DIR}/generated)

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
  ${CMAKE_CURRENT_LIST_DIR}/src/ltb/dd/*.cpp
  ${CMAKE_CURRENT_LIST_DIR}/src/ltb/ddd/*.cpp
  ${CMAKE_CURRENT_LIST_DIR}/src/ltb/cam/*.cpp
  ${CMAKE_CURRENT_LIST_DIR}/src/ltb/display/*.cpp
  ${CMAKE_CURRENT_LIST_DIR}/src/ltb/exec/*.cpp
  ${CMAKE_CURRENT_LIST_DIR}/src/ltb/geom/*.cpp
  ${CMAKE_CURRENT_LIST_DIR}/src/ltb/gui/*.cpp
  ${CMAKE_CURRENT_LIST_DIR}/src/ltb/utils/*.cpp
  ${CMAKE_CURRENT_LIST_DIR}/src/ltb/vlk/*.cpp
  ${CMAKE_CURRENT_LIST_DIR}/src/ltb/window/*.cpp
)

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
  range-v3::range-v3
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
target_compile_features(
  LtbVlk
  PUBLIC
  cxx_std_23
)
target_compile_definitions(
  LtbVlk
  PUBLIC
  # Deal with windows warnings and macros.
  $<$<PLATFORM_ID:Windows>:_CRT_SECURE_NO_WARNINGS>
  $<$<PLATFORM_ID:Windows>:NOMINMAX>
)
set_target_properties(
  LtbVlk
  PROPERTIES
  POSITION_INDEPENDENT_CODE ON
  WINDOWS_EXPORT_ALL_SYMBOLS ON
)
target_compile_options(
  LtbVlk
  PUBLIC
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
if (LTB_VLK_USE_STRICT_FLAGS)
  target_compile_options(
    LtbVlk
    PUBLIC
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

# ##############################################################################
# Applications
# ##############################################################################
if (LTB_VLK_BUILD_EXAMPLES)
  add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/src/ltb/apps)
endif ()
