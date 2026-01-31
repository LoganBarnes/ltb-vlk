// ///////////////////////////////////////////////////////////////////////////////////////
// A Logan Thomas Barnes project
// ///////////////////////////////////////////////////////////////////////////////////////
#include "ltb/vlk/command_pool.hpp"

// project
#include "ltb/vlk/check.hpp"
#include "ltb/vlk/device.hpp"
#include "ltb/vlk/physical_device.hpp"

// external
#include <spdlog/spdlog.h>

namespace ltb::vlk
{

CommandPool::CommandPool( Device& device, PhysicalDevice& physical_device )
    : device_( device )
    , physical_device_( physical_device )
{
}

auto CommandPool::initialize( ) -> utils::Result< void >
{
    return this->initialize( settings_ );
}

auto CommandPool::initialize( CommandPoolSettings const settings ) -> utils::Result< void >
{
    if ( this->is_initialized( ) )
    {
        return utils::success( );
    }
    LTB_CHECK_VALID( device_.is_initialized( ) );
    LTB_CHECK_VALID( physical_device_.is_initialized( ) );

    LTB_CHECK_VALID( physical_device_.queue_families( ).contains( settings.queue_type ) );
    auto const queue_family_index = physical_device_.queue_families( ).at( settings.queue_type );

    auto const command_pool_info = vk::CommandPoolCreateInfo{ }
                                       .setFlags( settings.flags )
                                       .setQueueFamilyIndex( queue_family_index );

    VK_CHECK( auto command_pool, device_.get( ).createCommandPoolUnique( command_pool_info ) );
    spdlog::debug( "vk::createCommandPoolUnique()" );

    settings_     = settings;
    command_pool_ = std::move( command_pool );

    return utils::success( );
}

auto CommandPool::is_initialized( ) const -> bool
{
    return nullptr != command_pool_.get( );
}

auto CommandPool::get( ) const -> vk::CommandPool const&
{
    return command_pool_.get( );
}

auto CommandPool::get( ) -> vk::CommandPool&
{
    return command_pool_.get( );
}

} // namespace ltb::vlk
