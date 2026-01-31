// ///////////////////////////////////////////////////////////////////////////////////////
// A Logan Thomas Barnes project
// ///////////////////////////////////////////////////////////////////////////////////////
#include "ltb/vlk/descriptor_pool.hpp"

// project
#include "ltb/vlk/check.hpp"
#include "ltb/vlk/device.hpp"

// external
#include <spdlog/spdlog.h>

namespace ltb::vlk
{

DescriptorPool::DescriptorPool( Device& device )
    : device_( device )
{
}

auto DescriptorPool::initialize( DescriptorPoolSettings settings ) -> utils::Result< void >
{
    if ( this->is_initialized( ) )
    {
        return utils::success( );
    }
    LTB_CHECK_VALID( device_.is_initialized( ) );

    auto const descriptor_pool_info = vk::DescriptorPoolCreateInfo{ }
                                          .setPoolSizes( settings.pool_sizes )
                                          .setMaxSets( settings.max_sets )
                                          .setFlags( settings.flags );
    VK_CHECK(
        auto descriptor_pool,
        device_.get( ).createDescriptorPoolUnique( descriptor_pool_info )
    );
    spdlog::debug( "vk::createDescriptorPoolUnique()" );

    settings_        = std::move( settings );
    descriptor_pool_ = std::move( descriptor_pool );

    return utils::success( );
}

auto DescriptorPool::is_initialized( ) const -> bool
{
    return nullptr != descriptor_pool_.get( );
}

auto DescriptorPool::get( ) const -> vk::DescriptorPool const&
{
    return descriptor_pool_.get( );
}

auto DescriptorPool::get( ) -> vk::DescriptorPool&
{
    return descriptor_pool_.get( );
}

auto DescriptorPool::settings( ) const -> DescriptorPoolSettings const&
{
    return settings_;
}

} // namespace ltb::vlk
