// ///////////////////////////////////////////////////////////////////////////////////////
// A Logan Thomas Barnes project
// ///////////////////////////////////////////////////////////////////////////////////////
#pragma once

// project
#include "ltb/vlk/objs/vulkan_buffer.hpp"

namespace ltb::vlk::dd
{

template < typename Uniforms >
struct MeshData
{
    objs::VulkanBuffer vbo;
    uint32             draw_count = 0U;
    Uniforms           uniforms   = { };

    explicit MeshData( objs::VulkanGpu& gpu )
        : vbo( gpu )
    {
    }
};

} // namespace ltb::vlk::dd
