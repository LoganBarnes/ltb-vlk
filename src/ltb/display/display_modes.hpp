// ///////////////////////////////////////////////////////////////////////////////////////
// A Logan Thomas Barnes project
// ///////////////////////////////////////////////////////////////////////////////////////
#pragma once

// project
#include "ltb/utils/types.hpp"

namespace ltb::display
{

enum class ColorMode : uint32
{
    White         = 0U,
    Positions     = 1U,
    Normals       = 2U,
    UvCoordinates = 3U,
    VertexColor   = 4U,
    UniformColor  = 5U,
};

enum class ShadingMode : uint32
{
    None,
    Simple,
};

} // namespace ltb::display
