// ///////////////////////////////////////////////////////////////////////////////////////
// A Logan Thomas Barnes project
// ///////////////////////////////////////////////////////////////////////////////////////
#pragma once

// project
#include "ltb/utils/result.hpp"
#include "ltb/vlk/fwd.hpp"
#include "ltb/vlk/vulkan.hpp"

namespace ltb::vlk
{

struct ImageSettings
{
    vk::ImageType           image_type     = vk::ImageType::e2D;
    vk::Extent3D            extent         = { };
    uint32_t                mip_levels     = 1;
    uint32_t                array_layers   = 1;
    vk::Format              format         = vk::Format::eUndefined;
    vk::ImageTiling         tiling         = vk::ImageTiling::eOptimal;
    vk::ImageLayout         initial_layout = vk::ImageLayout::eUndefined;
    vk::ImageUsageFlags     usage          = vk::ImageUsageFlagBits::eTransferDst;
    vk::SharingMode         sharing_mode   = vk::SharingMode::eExclusive;
    vk::SampleCountFlagBits samples        = vk::SampleCountFlagBits::e1;
};

class Image
{
public:
    explicit( false ) Image( Device& device );

    auto initialize( ImageSettings settings ) -> utils::Result< void >;

    [[nodiscard( "Const getter" )]]
    auto is_initialized( ) const -> bool;

    [[nodiscard( "Const getter" )]]
    auto get( ) const -> vk::Image const&;
    auto get( ) -> vk::Image&;

    auto reset( ) -> void;

    [[nodiscard( "Const getter" )]]
    auto settings( ) const -> ImageSettings const&;

    [[nodiscard( "Const getter" )]]
    auto memory_requirements( ) const -> vk::MemoryRequirements const&;

private:
    Device& device_;

    ImageSettings          settings_            = { };
    vk::UniqueImage        image_               = { };
    vk::MemoryRequirements memory_requirements_ = { };
};

} // namespace ltb::vlk
