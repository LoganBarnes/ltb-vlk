// ///////////////////////////////////////////////////////////////////////////////////////
// A Logan Thomas Barnes project
// ///////////////////////////////////////////////////////////////////////////////////////
#include "ltb/gui/imgui_context.hpp"

// external
#include <spdlog/spdlog.h>

namespace ltb::gui
{

auto ImguiContextDeleter::operator( )( ImGuiContext* const context ) const -> void
{
    if ( nullptr != context )
    {
        ImGui::DestroyContext( context );
    }
}

auto initialize_imgui_context( ) -> utils::Result< ImguiContextHandle >
{
    // Setup Dear ImGui binding
    IMGUI_CHECKVERSION( );

    if ( auto imgui = ImguiContextHandle( ImGui::CreateContext( ) ) )
    {
        spdlog::debug( "Created ImGui context" );
        return imgui;
    }

    return LTB_MAKE_UNEXPECTED_ERROR( "Failed to create ImGui context" );
}

} // namespace ltb::gui
