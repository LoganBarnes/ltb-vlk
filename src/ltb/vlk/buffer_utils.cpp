// ///////////////////////////////////////////////////////////////////////////////////////
// A Logan Thomas Barnes project
// ///////////////////////////////////////////////////////////////////////////////////////
#include "ltb/vlk/buffer_utils.hpp"

// project
#include "ltb/vlk/buffer.hpp"
#include "ltb/vlk/check.hpp"
#include "ltb/vlk/command_buffer.hpp"

// external
#include <spdlog/spdlog.h>

namespace ltb::vlk
{

auto copy_buffer(
    vk::CommandBuffer const&             command_buffer,
    vk::Queue const&                     submit_queue,
    Buffer const&                        src_buffer,
    Buffer const&                        dst_buffer,
    std::vector< vk::BufferCopy > const& buffer_regions
) -> utils::Result< void >
{
    VK_CHECK( command_buffer.begin(
        vk::CommandBufferBeginInfo{ }.setFlags( vk::CommandBufferUsageFlagBits::eOneTimeSubmit )
    ) );

    command_buffer.copyBuffer( src_buffer.get( ), dst_buffer.get( ), buffer_regions );

    VK_CHECK( command_buffer.end( ) );

    auto const submit_info = vk::SubmitInfo{ }.setCommandBuffers( command_buffer );

    VK_CHECK( submit_queue.submit( submit_info, nullptr ) );
    VK_CHECK( submit_queue.waitIdle( ) );

    return utils::success( );
}

auto copy_buffer(
    Device&                              device,
    CommandPool&                         command_pool,
    vk::Queue const&                     submit_queue,
    Buffer const&                        src_buffer,
    Buffer const&                        dst_buffer,
    std::vector< vk::BufferCopy > const& buffer_regions
) -> utils::Result< void >
{
    auto tmp_copy_command_buffer = CommandBuffer{ device, command_pool };
    LTB_CHECK( tmp_copy_command_buffer.initialize( ) );

    return copy_buffer(
        tmp_copy_command_buffer.get( ),
        submit_queue,
        src_buffer,
        dst_buffer,
        buffer_regions
    );
}

} // namespace ltb::vlk
