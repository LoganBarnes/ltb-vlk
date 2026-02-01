# ##############################################################################
# A Logan Thomas Barnes project
# ##############################################################################

### System Packages ###
find_package( Threads REQUIRED )

# Parallel execution policies work out of the box with MSVC.
# For GCC and Clang (and maybe Intel?) we need to find and
# link Intel's Thread Building Blocks (TBB) library.
if( NOT MSVC )
  # MacOS: brew install tbb
  # Linux: sudo apt update libtbb-dev
  find_package( TBB REQUIRED )
endif()

# Graphics APIs
find_package( Vulkan REQUIRED )

### External Repositories ###
cpmaddpackage(
  NAME
  cxxopts
  GITHUB_REPOSITORY
  jarro2783/cxxopts
  GIT_TAG
  v3.0.0
  DOWNLOAD_ONLY # header only
  TRUE
)
cpmaddpackage(
  NAME
  expected
  GIT_TAG
  ltb # master
  GITHUB_REPOSITORY
  LoganBarnes/expected
  OPTIONS
  "EXPECTED_BUILD_TESTS OFF"
)
cpmaddpackage(
  NAME
  GLFW
  GIT_REPOSITORY
  https://github.com/glfw/glfw.git
  GIT_TAG
  3.3.10
  OPTIONS
  "GLFW_BUILD_TESTS OFF"
  "GLFW_BUILD_EXAMPLES OFF"
  "GLFW_BUILD_DOCS OFF"
  "GLFW_INSTALL OFF"
)
cpmaddpackage(
  NAME
  glm
  # None of the tagged versions have the fixes we need
  GIT_TAG
  cc98465e3508535ba8c7f6208df934c156a018dc
  GITHUB_REPOSITORY
  g-truc/glm
  # It's header only and the CMakeLists.txt file adds
  # global flags that break CUDA on windows
  DOWNLOAD_ONLY
  TRUE
)
cpmaddpackage(
  NAME
  imgui
  GITHUB_REPOSITORY
  ocornut/imgui
  GIT_TAG
  396b33d # docking branch Dec 23, 2025
  DOWNLOAD_ONLY
  TRUE
)
cpmaddpackage( "gh:Neargye/magic_enum@0.7.3" )
cpmaddpackage(
  NAME
  range-v3
  GITHUB_REPOSITORY
  ericniebler/range-v3
  GIT_TAG
  0.12.0
  OPTIONS
  "RANGES_CXX_STD 20"
  # It's header only and the ThirdParty.cmake file adds
  # global flags that break CUDA on windows
  DOWNLOAD_ONLY
  TRUE
)
cpmaddpackage( "gh:gabime/spdlog@1.12.0" )

if( cxxopts_ADDED )
  add_library(
    cxxopts
    INTERFACE
  )
  add_library(
    cxxopts::cxxopts
    ALIAS
    cxxopts
  )
  # Mark external include directories as system includes to avoid warnings.
  target_include_directories(
    cxxopts
    SYSTEM
    INTERFACE
    $<BUILD_INTERFACE:${cxxopts_SOURCE_DIR}/include>
  )
endif()

if( magic_enum_ADDED )
  # Mark external include directories as system includes to avoid warnings.
  target_include_directories(
    magic_enum
    SYSTEM
    INTERFACE
    $<BUILD_INTERFACE:${magic_enum_SOURCE_DIR}/include>
  )
endif()

if( spdlog_ADDED )
  # Mark external include directories as system includes to avoid warnings.
  target_include_directories(
    spdlog
    SYSTEM
    INTERFACE
    $<BUILD_INTERFACE:${spdlog_SOURCE_DIR}/include>
  )
endif()

if( range-v3_ADDED )
  add_library(
    range-v3
    INTERFACE
  )
  add_library(
    range-v3::range-v3
    ALIAS
    range-v3
  )
  target_include_directories(
    range-v3
    SYSTEM
    INTERFACE
    # Mark external include directories as system includes to avoid warnings.
    $<BUILD_INTERFACE:${range-v3_SOURCE_DIR}/include>
  )
  target_compile_options(
    range-v3
    INTERFACE
    # Fix build warnings on Windows
    $<$<COMPILE_LANG_AND_ID:CXX,MSVC>:/permissive->
    $<$<COMPILE_LANG_AND_ID:CUDA,MSVC>:-Xcompiler=/permissive->
  )
endif()

if( glm_ADDED )
  add_library(
    glm
    INTERFACE
  )
  add_library(
    glm::glm
    ALIAS
    glm
  )
  target_compile_definitions(
    glm
    INTERFACE
    GLM_FORCE_RADIANS
    GLM_ENABLE_EXPERIMENTAL
    GLM_FORCE_EXPLICIT_CTOR
    #JustVulkanThings
    GLM_FORCE_DEPTH_ZERO_TO_ONE
  )
  target_include_directories(
    glm
    SYSTEM
    INTERFACE
    # Mark external include directories as system includes to avoid warnings.
    $<BUILD_INTERFACE:${glm_SOURCE_DIR}>
  )
  target_compile_features(
    glm
    INTERFACE
    cxx_std_17
  )
  target_compile_options(
    glm
    INTERFACE
    # Fix build warnings on Windows
    $<$<COMPILE_LANGUAGE:CUDA>:-Xcudafe --diag_suppress=esa_on_defaulted_function_ignored>
  )
endif()

if( GLFW_ADDED )
  add_library(
    glfw::glfw
    ALIAS
    glfw
  )
endif()

if( imgui_ADDED )
  add_library(
    imgui
    ${imgui_SOURCE_DIR}/imconfig.h
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui.h
    ${imgui_SOURCE_DIR}/imgui_demo.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_internal.h
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    ${imgui_SOURCE_DIR}/imstb_rectpack.h
    ${imgui_SOURCE_DIR}/imstb_textedit.h
    ${imgui_SOURCE_DIR}/imstb_truetype.h
    ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.h
    ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_vulkan.h
    ${imgui_SOURCE_DIR}/backends/imgui_impl_vulkan.cpp
  )
  add_library(
    imgui::imgui
    ALIAS
    imgui
  )
  target_link_libraries(
    imgui
    PUBLIC
    Vulkan::Vulkan
    glfw::glfw
  )
  target_include_directories(
    imgui
    SYSTEM
    PUBLIC
    # Mark external include directories as system includes to avoid warnings.
    $<BUILD_INTERFACE:${imgui_SOURCE_DIR}>
    $<BUILD_INTERFACE:${imgui_SOURCE_DIR}/backends>
  )
  target_compile_features(
    imgui
    PUBLIC
    cxx_std_11
  )
  target_compile_options(
    imgui
    PRIVATE
    # Ignore build warning on this third-party code
    $<$<COMPILE_LANG_AND_ID:CXX,GNU,Clang,AppleClang>:-w>
  )
endif()
