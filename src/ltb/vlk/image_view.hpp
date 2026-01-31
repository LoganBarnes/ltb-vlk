// ///////////////////////////////////////////////////////////////////////////////////////
// A Logan Thomas Barnes project
// ///////////////////////////////////////////////////////////////////////////////////////
#pragma once

// project
#include "ltb/utils/result.hpp"
#include "ltb/utils/types.hpp"
#include "ltb/vlk/fwd.hpp"
#include "ltb/vlk/vulkan.hpp"

namespace ltb::vlk
{

struct ImageViewSettings
{
    vk::Image            image            = nullptr;
    vk::ImageViewType    image_type       = vk::ImageViewType::e2D;
    vk::Format           format           = vk::Format::eUndefined;
    vk::ComponentMapping components       = { };
    vk::ImageAspectFlags aspect_mask      = vk::ImageAspectFlagBits::eColor;
    uint32               base_mip_level   = 0U;
    uint32               level_count      = 1U;
    uint32               base_array_layer = 0U;
    uint32               layer_count      = 1U;
};

class ImageView
{
public:
    explicit( false ) ImageView( Device& device );

    auto initialize( ImageViewSettings settings ) -> utils::Result< void >;

    [[nodiscard( "Const getter" )]]
    auto is_initialized( ) const -> bool;

    [[nodiscard( "Const getter" )]]
    auto get( ) const -> vk::ImageView const&;
    auto get( ) -> vk::ImageView&;

    auto reset( ) -> void;

    [[nodiscard( "Const getter" )]]
    auto settings( ) const -> ImageViewSettings const&;

private:
    Device& device_;

    ImageViewSettings   settings_   = { };
    vk::UniqueImageView image_view_ = { };
};

} // namespace ltb::vlk
