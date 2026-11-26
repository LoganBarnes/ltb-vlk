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
    LTB_CHECK_VALID(
        ( vk::SharingMode::eConcurrent != settings.sharing_mode )
            || ( settings.queue_family_indices.size( ) > 1U ),
        "concurrent sharing needs at least two queue family indices"
    );

    auto image_info = vk::ImageCreateInfo{ }
                          .setFlags( settings.flags )
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
    if ( vk::SharingMode::eConcurrent == settings.sharing_mode )
    {
        image_info.setQueueFamilyIndices( settings.queue_family_indices );
    }

    image_info.setPNext( settings.p_next );

    auto const external_memory_info = vk::ExternalMemoryImageCreateInfo{ }
                                          .setPNext( settings.p_next )
                                          .setHandleTypes( settings.external_memory_handle_types );
    if ( settings.external_memory_handle_types )
    {
        image_info.setPNext( &external_memory_info );
    }

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
