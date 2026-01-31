// ///////////////////////////////////////////////////////////////////////////////////////
// A Logan Thomas Barnes project
// ///////////////////////////////////////////////////////////////////////////////////////
#pragma once

// project
#include "ltb/utils/result.hpp"
#include "ltb/utils/types.hpp"
#include "ltb/vlk/fwd.hpp"
#include "ltb/vlk/queue_types.hpp"
#include "ltb/vlk/vulkan.hpp"

// standard
#include <set>

namespace ltb::vlk
{

using PhysicalDeviceFeature            = vk::Bool32            vk::PhysicalDeviceFeatures::*;
using PhysicalDeviceFeature2           = vk::Bool32           vk::PhysicalDeviceFeatures2::*;
using PhysicalDeviceRobustness2Feature = vk::Bool32 vk::PhysicalDeviceRobustness2FeaturesEXT::*;

struct DeviceSettings
{
    std::vector< vk::QueueFlagBits > queue_flags = {
        vk::QueueFlagBits::eGraphics,
        vk::QueueFlagBits::eCompute,
    };

    std::vector< char const* > extensions = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME,
#if defined( __APPLE__ )
        "VK_KHR_portability_subset",
#endif
    };

    std::vector< char const* > optional_extensions = {
        VK_KHR_PRESENT_MODE_FIFO_LATEST_READY_EXTENSION_NAME,
    };

    std::vector< PhysicalDeviceFeature > device_features = {
        &vk::PhysicalDeviceFeatures::samplerAnisotropy,
        &vk::PhysicalDeviceFeatures::fillModeNonSolid,
    };

    std::vector< vk::Format > preferred_depth_formats = {
        vk::Format::eD32Sfloat,
        vk::Format::eD32SfloatS8Uint,
        vk::Format::eD24UnormS8Uint,
    };

    vk::ImageTiling preferred_depth_tiling = vk::ImageTiling::eOptimal;
};

class PhysicalDevice
{
public:
    explicit( false ) PhysicalDevice( Instance& instance );

    auto initialize( DeviceSettings settings, Surface const* surface ) -> utils::Result< void >;

    [[nodiscard( "Const getter" )]]
    auto is_initialized( ) const -> bool;

    [[nodiscard( "Const getter" )]]
    auto get( ) const -> vk::PhysicalDevice const&;
    auto get( ) -> vk::PhysicalDevice&;

    [[nodiscard( "Const getter" )]]
    auto settings( ) const -> DeviceSettings const&;

    [[nodiscard( "Const getter" )]]
    auto extensions( ) const -> std::vector< char const* > const&;

    [[nodiscard( "Const getter" )]]
    auto properties( ) const -> vk::PhysicalDeviceProperties const&;

    [[nodiscard( "Const getter" )]]
    auto queue_families( ) const -> QueueFamilyMap const&;

    [[nodiscard( "Const getter" )]]
    auto unique_queue_families( ) const -> std::set< QueueIndex > const&;

    [[nodiscard( "Const getter" )]]
    auto depth_image_format( ) const -> vk::Format;

    [[nodiscard( "Const computation" )]]
    auto find_memory_type_index(
        std::vector< vk::MemoryRequirements > const& memory_requirements,
        vk::MemoryPropertyFlags                      memory_properties
    ) const -> utils::Result< uint32 >;

private:
    Instance& instance_;

    DeviceSettings               settings_              = { };
    vk::PhysicalDevice           physical_device_       = { };
    std::vector< char const* >   extensions_            = { };
    vk::PhysicalDeviceProperties properties_            = { };
    QueueFamilyMap               queue_families_        = { };
    std::set< QueueIndex >       unique_queue_families_ = { };
    vk::Format                   depth_image_format_    = vk::Format::eUndefined;
};

} // namespace ltb::vlk
