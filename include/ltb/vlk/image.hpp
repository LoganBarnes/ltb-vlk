// ///////////////////////////////////////////////////////////////////////////////////////
// A Logan Thomas Barnes project
// ///////////////////////////////////////////////////////////////////////////////////////
#pragma once

// project
#include "ltb/utils/result.hpp"
#include "ltb/utils/types.hpp"
#include "ltb/vlk/fwd.hpp"
#include "ltb/vlk/vulkan.hpp"

// standard
#include <vector>

namespace ltb::vlk
{

struct ImageSettings
{
    vk::ImageCreateFlags    flags          = { };
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

    /// \brief The queue families that will access the image. Only used (and required, with
    ///        at least two unique families) when `sharing_mode` is `eConcurrent`.
    std::vector< uint32 > queue_family_indices = { };

    /// \brief Set to export or import the image's memory (e.g. `eOpaqueFd` to share it with
    ///        another process). Images that share memory must use identical settings.
    vk::ExternalMemoryHandleTypeFlags external_memory_handle_types = { };

    /// \brief Extra structs to chain onto `vk::ImageCreateInfo`, for platform-specific
    ///        options without a setting here (e.g. `vk::ImportMetalIOSurfaceInfoEXT`).
    void const* p_next = nullptr;
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
