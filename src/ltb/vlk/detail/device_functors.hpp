// ///////////////////////////////////////////////////////////////////////////////////////
// A Logan Thomas Barnes project
// ///////////////////////////////////////////////////////////////////////////////////////
#pragma once

// project
#include "ltb/utils/types.hpp"
#include "ltb/vlk/physical_device.hpp"
#include "ltb/vlk/queue_types.hpp"
#include "ltb/vlk/vulkan.hpp"

namespace ltb::vlk::detail
{

struct CheckExtensionSupport
{
    std::vector< char const* > const& extensions;

    auto operator( )( vk::PhysicalDevice const& physical_device ) const
        -> utils::Result< vk::PhysicalDevice >;
};

struct AppendSupportedExtensions
{
    std::vector< char const* >&       extensions;
    std::vector< char const* > const& optional_extensions;

    auto operator( )( vk::PhysicalDevice const& physical_device ) const
        -> utils::Result< vk::PhysicalDevice >;
};

struct CheckFeatureSupport
{
    std::vector< PhysicalDeviceFeature > const& features;

    auto operator( )( vk::PhysicalDevice const& physical_device ) const
        -> utils::Result< vk::PhysicalDevice >;
};

struct CheckSurfaceSupport
{
    vk::SurfaceKHR const& surface;

    auto operator( )( vk::PhysicalDevice const& physical_device ) const
        -> utils::Result< vk::PhysicalDevice >;
};

struct SuitablePhysicalDevice
{
    vk::PhysicalDevice         physical_device;
    QueueFamilyMap             queue_families;
    std::vector< char const* > extensions;

    explicit SuitablePhysicalDevice(
        vk::PhysicalDevice         potential_device,
        QueueFamilyMap             queue_family_map,
        std::vector< char const* > supported_extensions
    );

    uint32 score = 0U;
};

inline auto operator<=>( SuitablePhysicalDevice const& lhs, SuitablePhysicalDevice const& rhs )
{
    return lhs.score <=> rhs.score;
}

struct IsSuitableMemoryType
{
    uint32 const&                             index;
    vk::PhysicalDeviceMemoryProperties const& device_memory_properties;
    vk::MemoryPropertyFlags const&            properties;

    auto operator( )( vk::MemoryRequirements const& mem_req ) const -> bool
    {
        auto const is_suitable_type = ( mem_req.memoryTypeBits & ( 1U << index ) );
        auto const desired_properties_exist
            = ( ( device_memory_properties.memoryTypes[ index ].propertyFlags & properties )
                == properties );

        return is_suitable_type && desired_properties_exist;
    }
};

} // namespace ltb::vlk::detail
