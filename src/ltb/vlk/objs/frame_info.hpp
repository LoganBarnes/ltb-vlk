// ///////////////////////////////////////////////////////////////////////////////////////
// A Logan Thomas Barnes project
// ///////////////////////////////////////////////////////////////////////////////////////
#pragma once

// project
#include "ltb/utils/types.hpp"
#include "ltb/vlk/vulkan.hpp"

// standard
#include <limits>

namespace ltb::vlk::objs
{

struct FrameInfo
{
    vk::CommandBuffer command_buffer = nullptr;
    vk::Fence         frame_fence    = nullptr;
    uint32            frame_index    = std::numeric_limits< uint32 >::max( );

    vk::Semaphore image_semaphore = nullptr;
    uint32        image_index     = std::numeric_limits< uint32 >::max( );
};

} // namespace ltb::vlk::objs
