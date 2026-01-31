// ///////////////////////////////////////////////////////////////////////////////////////
// A Logan Thomas Barnes project
// ///////////////////////////////////////////////////////////////////////////////////////
#include "ltb/vlk/buffer.hpp"

// project
#include "ltb/vlk/check.hpp"
#include "ltb/vlk/command_pool.hpp"
#include "ltb/vlk/device.hpp"

// external
#include <spdlog/spdlog.h>

namespace ltb::vlk
{

Buffer::Buffer( Device& device )
    : device_( device )
{
}

auto Buffer::initialize( BufferSettings settings ) -> utils::Result< void >
{
    if ( this->is_initialized( ) )
    {
        return utils::success( );
    }
    LTB_CHECK_VALID( device_.is_initialized( ) );

    auto const buffer_info = vk::BufferCreateInfo{ }
                                 .setSize( settings.size )
                                 .setUsage( settings.usage )
                                 .setSharingMode( settings.sharing_mode )
                                 .setFlags( settings.flags );

    VK_CHECK( auto buffer, device_.get( ).createBufferUnique( buffer_info ) );
    spdlog::debug( "vk::createBufferUnique()" );

    auto const memory_requirements = device_.get( ).getBufferMemoryRequirements( buffer.get( ) );

    settings_            = std::move( settings );
    buffer_              = std::move( buffer );
    memory_requirements_ = memory_requirements;

    return utils::success( );
}

auto Buffer::is_initialized( ) const -> bool
{
    return nullptr != buffer_.get( );
}

auto Buffer::get( ) const -> vk::Buffer const&
{
    return buffer_.get( );
}

auto Buffer::get( ) -> vk::Buffer&
{
    return buffer_.get( );
}

auto Buffer::reset( ) -> void
{
    memory_requirements_ = vk::MemoryRequirements{ };
    buffer_.reset( );
    settings_ = { };
}

auto Buffer::settings( ) const -> BufferSettings const&
{
    return settings_;
}

auto Buffer::memory_requirements( ) const -> vk::MemoryRequirements const&
{
    return memory_requirements_;
}

} // namespace ltb::vlk
