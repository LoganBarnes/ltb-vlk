// ///////////////////////////////////////////////////////////////////////////////////////
// A Logan Thomas Barnes project
// ///////////////////////////////////////////////////////////////////////////////////////
#pragma once

// project
#include "ltb/display/mesh_display_settings.hpp"

// external
#include <glm/glm.hpp>

namespace ltb::display
{

struct DisplayUniforms
{
    alignas( 16 ) glm::vec4 color         = glm::vec4( 1.f );
    alignas( 4 ) ColorMode color_mode     = ColorMode::UniformColor;
    alignas( 4 ) ShadingMode shading_mode = ShadingMode::Flat;
    // 8 bytes padding
};

static_assert( sizeof( DisplayUniforms ) == 16 + 4 + 4 + 8 );
static_assert( alignof( DisplayUniforms ) == 16 );

} // namespace ltb::display
