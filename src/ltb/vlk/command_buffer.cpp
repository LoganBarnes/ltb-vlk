// ///////////////////////////////////////////////////////////////////////////////////////
// A Logan Thomas Barnes project
// ///////////////////////////////////////////////////////////////////////////////////////
#include "ltb/vlk/command_buffer.hpp"

// project
#include "ltb/vlk/check.hpp"
#include "ltb/vlk/command_pool.hpp"
#include "ltb/vlk/device.hpp"

// external
#include <spdlog/spdlog.h>

namespace ltb::vlk
{

CommandBuffer::CommandBuffer( Device& device, CommandPool& command_pool )
    : device_( device )
    , command_pool_( command_pool )
{
}

auto CommandBuffer::initialize( ) -> utils::Result< void >
{
    return this->initialize( settings_ );
}

auto CommandBuffer::initialize( CommandBufferSettings const settings ) -> utils::Result< void >
{
    if ( this->is_initialized( ) )
    {
        return utils::success( );
    }
    LTB_CHECK_VALID( device_.is_initialized( ) );
    LTB_CHECK_VALID( command_pool_.is_initialized( ) );

    auto const command_buffer_info = vk::CommandBufferAllocateInfo{ }
                                         .setCommandPool( command_pool_.get( ) )
                                         .setLevel( settings_.level )
                                         .setCommandBufferCount( 1U );
    VK_CHECK(
        auto command_buffers,
        device_.get( ).allocateCommandBuffersUnique( command_buffer_info )
    );
    spdlog::debug( "vk::createCommandBufferUnique()" );

    settings_       = settings;
    command_buffer_ = std::move( command_buffers.front( ) );

    return utils::success( );
}

auto CommandBuffer::is_initialized( ) const -> bool
{
    return nullptr != command_buffer_.get( );
}

auto CommandBuffer::get( ) const -> vk::CommandBuffer const&
{
    return command_buffer_.get( );
}

auto CommandBuffer::get( ) -> vk::CommandBuffer&
{
    return command_buffer_.get( );
}

} // namespace ltb::vlk
