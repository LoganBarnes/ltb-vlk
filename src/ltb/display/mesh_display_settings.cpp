// ///////////////////////////////////////////////////////////////////////////////////////
// A Logan Thomas Barnes project
// ///////////////////////////////////////////////////////////////////////////////////////
#include "ltb/display/mesh_display_settings.hpp"

namespace ltb::display
{

auto update_settings(
    MeshDisplaySettings3d               settings,
    PartialMeshDisplaySettings3d const& partial_settings
) -> MeshDisplaySettings3d
{
    if ( partial_settings.visible.has_value( ) )
    {
        settings.visible = partial_settings.visible.value( );
    }
    if ( partial_settings.mesh_format.has_value( ) )
    {
        settings.mesh_format = partial_settings.mesh_format.value( );
    }
    if ( partial_settings.color_mode.has_value( ) )
    {
        settings.color_mode = partial_settings.color_mode.value( );
    }
    if ( partial_settings.uniform_color.has_value( ) )
    {
        settings.uniform_color = partial_settings.uniform_color.value( );
    }
    if ( partial_settings.shading_mode.has_value( ) )
    {
        settings.shading_mode = partial_settings.shading_mode.value( );
    }
    if ( partial_settings.transforms.has_value( ) )
    {
        settings.transforms = partial_settings.transforms.value( );
    }
    if ( partial_settings.draw_count.has_value( ) )
    {
        settings.draw_count = partial_settings.draw_count.value( );
    }

    return settings;
}

} // namespace ltb::display
