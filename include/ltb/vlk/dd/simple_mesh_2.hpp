// ///////////////////////////////////////////////////////////////////////////////////////
// A Logan Thomas Barnes project
// ///////////////////////////////////////////////////////////////////////////////////////
#pragma once

// project
#include "ltb/utils/types.hpp"
#include "ltb/vlk/vulkan.hpp"

// external
#include <glm/ext/matrix_transform.hpp>
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

struct SimpleModelUniforms
{
    glm::mat3x4 transform = glm::identity< glm::mat3x4 >( );
};
static_assert(sizeof(SimpleModelUniforms) == 48U);

struct SimpleDisplayUniforms
{
    glm::vec4 color = glm::vec4( 1.0F );
};
static_assert(sizeof(SimpleDisplayUniforms) == 16U);

struct SimpleMeshUniforms
{
    SimpleModelUniforms   model   = { };
    SimpleDisplayUniforms display = { };
};

} // namespace ltb::vlk::dd
