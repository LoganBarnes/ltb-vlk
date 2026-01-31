// ///////////////////////////////////////////////////////////////////////////////////////
// A Logan Thomas Barnes project
// ///////////////////////////////////////////////////////////////////////////////////////
#include "ltb/vlk/descriptor_set_layout.hpp"

// project
#include "ltb/vlk/check.hpp"
#include "ltb/vlk/device.hpp"

// external
#include <spdlog/spdlog.h>

namespace ltb::vlk
{

DescriptorSetLayout::DescriptorSetLayout( Device& device )
    : device_( device )
{
}

auto DescriptorSetLayout::initialize( DescriptorSetLayoutSettings settings )
    -> utils::Result< void >
{
    if ( this->is_initialized( ) )
    {
        return utils::success( );
    }
    LTB_CHECK_VALID( device_.is_initialized( ) );

    auto const create_info = vk::DescriptorSetLayoutCreateInfo{ }.setBindings( settings.bindings );

    VK_CHECK(
        auto descriptor_set_layout,
        device_.get( ).createDescriptorSetLayoutUnique( create_info )
    );
    spdlog::debug( "vk::createDescriptorSetLayoutUnique()" );

    settings_              = std::move( settings );
    descriptor_set_layout_ = std::move( descriptor_set_layout );

    return utils::success( );
}

auto DescriptorSetLayout::is_initialized( ) const -> bool
{
    return nullptr != descriptor_set_layout_.get( );
}

auto DescriptorSetLayout::get( ) const -> vk::DescriptorSetLayout const&
{
    return descriptor_set_layout_.get( );
}

auto DescriptorSetLayout::get( ) -> vk::DescriptorSetLayout&
{
    return descriptor_set_layout_.get( );
}

} // namespace ltb::vlk
