// ///////////////////////////////////////////////////////////////////////////////////////
// A Logan Thomas Barnes project
// ///////////////////////////////////////////////////////////////////////////////////////
#pragma once

// project
#include "ltb/utils/types.hpp"
#include "ltb/vlk/vulkan.hpp"

// external
#include <glm/glm.hpp>

namespace ltb::vlk::dd
{

struct SimpleMesh2
{
    using PositionType                        = glm::vec2;
    static constexpr auto position_size_bytes = static_cast< uint32 >( sizeof( PositionType ) );
    static constexpr auto position_format     = vk::Format::eR32G32Sfloat;

    std::vector< PositionType > positions = { };
};

struct SimpleDisplayUniforms
{
    alignas( 16 ) glm::vec4 color = glm::vec4( 1.0F );
};

static_assert( sizeof( SimpleDisplayUniforms ) == 16 );
static_assert( alignof( SimpleDisplayUniforms ) == 16 );

} // namespace ltb::vlk::dd
