// ///////////////////////////////////////////////////////////////////////////////////////
// A Logan Thomas Barnes project
// ///////////////////////////////////////////////////////////////////////////////////////
#include "ltb/gui/imgui_glfw.hpp"

// external
#include <imgui_impl_glfw.h>

namespace ltb::gui
{

ScopedImguiGlfw::ScopedImguiGlfw( GLFWwindow* const window )
    : init_value{ ImGui_ImplGlfw_InitForOpenGL( window, true ) }
{
}

ScopedImguiGlfw::~ScopedImguiGlfw( )
{
    if ( init_value )
    {
        ::ImGui_ImplGlfw_Shutdown( );
    }
}

auto initialize_imgui_glfw( GLFWwindow* const window ) -> utils::Result< ImguiGlfwHandle >
{
    if ( auto imgui_glfw = std::make_unique< ScopedImguiGlfw >( window ); imgui_glfw->init_value )
    {
        return imgui_glfw;
    }
    return LTB_MAKE_UNEXPECTED_ERROR( "ImGui_ImplGlfw_InitForOpenGL() failed" );
}

} // namespace ltb::gui
