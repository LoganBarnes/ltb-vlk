// ///////////////////////////////////////////////////////////////////////////////////////
// A Logan Thomas Barnes project
// ///////////////////////////////////////////////////////////////////////////////////////
#include "ltb/vlk/image.hpp"

// project
#include "ltb/vlk/check.hpp"
#include "ltb/vlk/device.hpp"

// external
#include <spdlog/spdlog.h>

namespace ltb::vlk
{

Image::Image( Device& device )
    : device_( device )
{
}

auto Image::initialize( ImageSettings settings ) -> utils::Result< void >
{
    if ( this->is_initialized( ) )
    {
        return utils::success( );
    }
    LTB_CHECK_VALID( device_.is_initialized( ) );

    auto const image_info = vk::ImageCreateInfo{ }
                                .setImageType( settings.image_type )
                                .setExtent( settings.extent )
                                .setMipLevels( settings.mip_levels )
                                .setArrayLayers( settings.array_layers )
                                .setFormat( settings.format )
                                .setTiling( settings.tiling )
                                .setInitialLayout( settings.initial_layout )
                                .setUsage( settings.usage )
                                .setSharingMode( settings.sharing_mode )
                                .setSamples( settings.samples );

    VK_CHECK( auto image, device_.get( ).createImageUnique( image_info ) );
    spdlog::debug( "vk::createImageUnique()" );

    auto const memory_requirements = device_.get( ).getImageMemoryRequirements( image.get( ) );

    settings_            = std::move( settings );
    image_               = std::move( image );
    memory_requirements_ = memory_requirements;

    return utils::success( );
}

auto Image::is_initialized( ) const -> bool
{
    return nullptr != image_.get( );
}

auto Image::get( ) const -> vk::Image const&
{
    return image_.get( );
}

auto Image::get( ) -> vk::Image&
{
    return image_.get( );
}

auto Image::reset( ) -> void
{
    memory_requirements_ = vk::MemoryRequirements{ };
    image_               = { };
    settings_            = { };
}

auto Image::settings( ) const -> ImageSettings const&
{
    return settings_;
}

auto Image::memory_requirements( ) const -> vk::MemoryRequirements const&
{
    return memory_requirements_;
}

} // namespace ltb::vlk
