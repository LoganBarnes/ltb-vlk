// ///////////////////////////////////////////////////////////////////////////////////////
// A Logan Thomas Barnes project
// ///////////////////////////////////////////////////////////////////////////////////////
#pragma once

// project
#include "ltb/utils/result.hpp"
#include "ltb/utils/types.hpp"
#include "ltb/vlk/vulkan.hpp"

namespace ltb::vlk
{

enum class QueueType
{
    Unknown,
    Graphics,
    Compute,
    Transfer,
    SparseBinding,
    Protected,
    Surface,
};

[[nodiscard( "Const conversion" )]]
auto to_queue_type( vk::QueueFlagBits const& queue_type ) -> QueueType;

using QueueIndex     = uint32;
using QueueFamilyMap = std::unordered_map< QueueType, QueueIndex >;
using QueueMap       = std::unordered_map< QueueType, vk::Queue >;

auto build_queue_family_map(
    vk::PhysicalDevice const&               physical_device,
    std::vector< vk::QueueFlagBits > const& types,
    vk::SurfaceKHR const&                   surface
) -> utils::Result< QueueFamilyMap >;

struct BuildQueueFamilyMap
{
    std::vector< vk::QueueFlagBits > const& types;
    vk::SurfaceKHR const&                   surface;

    auto operator( )( vk::PhysicalDevice const& physical_device ) const
        -> utils::Result< QueueFamilyMap >
    {
        return build_queue_family_map( physical_device, types, surface );
    }
};

} // namespace ltb::vlk
