// ///////////////////////////////////////////////////////////////////////////////////////
// A Logan Thomas Barnes project
// ///////////////////////////////////////////////////////////////////////////////////////
#include "ltb/vlk/image_view.hpp"

// project
#include "ltb/vlk/check.hpp"
#include "ltb/vlk/device.hpp"

// external
#include <spdlog/spdlog.h>

namespace ltb::vlk
{

ImageView::ImageView( Device& device )
    : device_( device )
{
}

auto ImageView::initialize( ImageViewSettings settings ) -> utils::Result< void >
{
    if ( this->is_initialized( ) )
    {
        return utils::success( );
    }
    LTB_CHECK_VALID( device_.is_initialized( ) );
    LTB_CHECK_VALID( nullptr != settings.image );

    auto const create_info = vk::ImageViewCreateInfo{ }
                                 .setImage( settings.image )
                                 .setViewType( settings.image_type )
                                 .setFormat( settings.format )
                                 .setComponents( settings.components )
                                 .setSubresourceRange(
                                     vk::ImageSubresourceRange{ }
                                         .setAspectMask( settings.aspect_mask )
                                         .setBaseMipLevel( settings.base_mip_level )
                                         .setLevelCount( settings.level_count )
                                         .setBaseArrayLayer( settings.base_array_layer )
                                         .setLayerCount( settings.layer_count )
                                 );

    VK_CHECK( auto image_view, device_.get( ).createImageViewUnique( create_info ) );
    spdlog::debug( "vk::createImageViewUnique()" );

    settings_   = std::move( settings );
    image_view_ = std::move( image_view );

    return utils::success( );
}

auto ImageView::is_initialized( ) const -> bool
{
    return nullptr != image_view_.get( );
}

auto ImageView::get( ) const -> vk::ImageView const&
{
    return image_view_.get( );
}

auto ImageView::get( ) -> vk::ImageView&
{
    return image_view_.get( );
}

auto ImageView::reset( ) -> void
{
    image_view_ = { };
    settings_   = { };
}

auto ImageView::settings( ) const -> ImageViewSettings const&
{
    return settings_;
}

} // namespace ltb::vlk
