// ///////////////////////////////////////////////////////////////////////////////////////
// A Logan Thomas Barnes project
// ///////////////////////////////////////////////////////////////////////////////////////
#pragma once

// project
#include "ltb/utils/result.hpp"
#include "ltb/vlk/fwd.hpp"
#include "ltb/vlk/vulkan.hpp"

namespace ltb::vlk
{

auto copy_buffer(
    vk::CommandBuffer const&             command_buffer,
    vk::Queue const&                     submit_queue,
    Buffer const&                        src_buffer,
    Buffer const&                        dst_buffer,
    std::vector< vk::BufferCopy > const& buffer_regions
) -> utils::Result< void >;

auto copy_buffer(
    Device&                              device,
    CommandPool&                         command_pool,
    vk::Queue const&                     submit_queue,
    Buffer const&                        src_buffer,
    Buffer const&                        dst_buffer,
    std::vector< vk::BufferCopy > const& buffer_regions
) -> utils::Result< void >;

} // namespace ltb::vlk
