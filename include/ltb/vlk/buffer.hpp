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

struct BufferSettings
{
    vk::BufferCreateFlags flags        = { };
    vk::DeviceSize        size         = 0U;
    vk::BufferUsageFlags  usage        = vk::BufferUsageFlagBits::eVertexBuffer;
    vk::SharingMode       sharing_mode = vk::SharingMode::eExclusive;

    /// \brief The queue families that will access the buffer. Only used (and required, with
    ///        at least two unique families) when `sharing_mode` is `eConcurrent`.
    std::vector< uint32 > queue_family_indices = { };
};

class Buffer
{
public:
    explicit( false ) Buffer( Device& device );

    auto initialize( BufferSettings settings ) -> utils::Result< void >;

    [[nodiscard( "Const getter" )]]
    auto is_initialized( ) const -> bool;

    [[nodiscard( "Const getter" )]]
    auto get( ) const -> vk::Buffer const&;
    auto get( ) -> vk::Buffer&;

    auto reset( ) -> void;

    [[nodiscard( "Const getter" )]]
    auto settings( ) const -> BufferSettings const&;

    [[nodiscard( "Const getter" )]]
    auto memory_requirements( ) const -> vk::MemoryRequirements const&;

private:
    Device& device_;

    BufferSettings         settings_            = { };
    vk::UniqueBuffer       buffer_              = { };
    vk::MemoryRequirements memory_requirements_ = { };
};

} // namespace ltb::vlk
