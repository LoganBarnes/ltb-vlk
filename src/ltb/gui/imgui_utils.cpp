// ///////////////////////////////////////////////////////////////////////////////////////
// A Logan Thomas Barnes project
// ///////////////////////////////////////////////////////////////////////////////////////
#include "ltb/gui/imgui_utils.hpp"

namespace ltb::gui
{

auto get_window_content_bounds( ) -> geom::Range2
{
    auto const start_pos      = glm::vec2( ImGui::GetCursorScreenPos( ) );
    auto const available_area = glm::vec2( ImGui::GetContentRegionAvail( ) );

    return {
        .min = start_pos,
        .max = start_pos + available_area,
    };
}

ScopedId::ScopedId( char const* str_id )
{
    ImGui::PushID( str_id );
}

ScopedId::ScopedId( char const* str_id_begin, char const* str_id_end )
{
    ImGui::PushID( str_id_begin, str_id_end );
}

ScopedId::ScopedId( int int_id )
{
    ImGui::PushID( int_id );
}

ScopedId::~ScopedId( )
{
    ImGui::PopID( );
}

} // namespace ltb::gui
