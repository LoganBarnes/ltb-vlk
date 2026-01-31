// ///////////////////////////////////////////////////////////////////////////////////////
// A Logan Thomas Barnes project
// ///////////////////////////////////////////////////////////////////////////////////////
#pragma once

// project
#include "ltb/utils/result.hpp"

// external
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

namespace ltb::gui
{

class ScopedImguiGlfw
{
public:
    explicit ScopedImguiGlfw( GLFWwindow* window );
    ~ScopedImguiGlfw( );

    bool init_value;
};

using ImguiGlfwHandle = std::unique_ptr< ScopedImguiGlfw >;

auto initialize_imgui_glfw( GLFWwindow* window ) -> utils::Result< ImguiGlfwHandle >;

} // namespace ltb::gui
