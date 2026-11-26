// ///////////////////////////////////////////////////////////////////////////////////////
// A Logan Thomas Barnes project
// ///////////////////////////////////////////////////////////////////////////////////////
#include "ltb/vlk/physical_device.hpp"

// project
#include "ltb/vlk/check.hpp"
#include "ltb/vlk/detail/device_functors.hpp"
#include "ltb/vlk/instance.hpp"
#include "ltb/vlk/surface.hpp"

// external
#include <spdlog/fmt/ranges.h>
#include <spdlog/spdlog.h>

// standard
#include <algorithm>
#include <queue>
#include <ranges>
#include <string_view>

namespace ltb::vlk
{
namespace
{

auto find_supported_format(
    vk::PhysicalDevice const&        physical_device,
    std::vector< vk::Format > const& candidates,
    vk::ImageTiling const            tiling,
    vk::FormatFeatureFlags const     features
) -> utils::Result< vk::Format >
{
    for ( auto const& format : candidates )
    {
        auto const properties = physical_device.getFormatProperties( format );

        switch ( tiling )
        {
            using enum vk::ImageTiling;
            case eLinear:
                if ( ( properties.linearTilingFeatures & features ) == features )
                {
                    return format;
                }
                break;

            case eOptimal:
                if ( ( properties.optimalTilingFeatures & features ) == features )
                {
                    return format;
                }
                break;

            default:
                break;
        }
    }

    return LTB_MAKE_UNEXPECTED_ERROR( "Failed to find supported format" );
}

} // namespace

PhysicalDevice::PhysicalDevice( Instance& instance )
    : instance_( instance )
{
}

auto PhysicalDevice::initialize( DeviceSettings settings, Surface const* const surface )
    -> utils::Result< void >
{
    if ( this->is_initialized( ) )
    {
        return utils::success( );
    }
    LTB_CHECK_VALID( instance_.is_initialized( ) );

    if ( nullptr != surface )
    {
        LTB_CHECK_VALID( surface->is_initialized( ) );
    }

    VK_CHECK( auto const physical_devices, instance_.get( ).enumeratePhysicalDevices( ) );
    if ( physical_devices.empty( ) )
    {
        return LTB_MAKE_UNEXPECTED_ERROR( "No physical devices found" );
    }

    auto vk_surface = vk::SurfaceKHR{ nullptr };
    if ( nullptr != surface )
    {
        vk_surface = surface->get( );
    }

    auto sorted_devices = std::priority_queue< detail::SuitablePhysicalDevice >{ };
    auto error_messages = std::vector< std::string >{ };

    for ( auto const& physical_device : physical_devices )
    {
        auto extensions          = settings.extensions;
        auto optional_extensions = settings.optional_extensions;

        // Portability drivers (MoltenVK) require VK_KHR_portability_subset to be enabled,
        // so add it even when the caller replaced the default extension list.
        constexpr auto portability_subset = std::string_view{ "VK_KHR_portability_subset" };
        auto const     is_portability_subset = [ portability_subset ]( char const* const name ) {
            return std::string_view{ name } == portability_subset;
        };
        if ( std::ranges::none_of( extensions, is_portability_subset )
             && std::ranges::none_of( optional_extensions, is_portability_subset ) )
        {
            optional_extensions.push_back( portability_subset.data( ) );
        }

        if ( auto queue_families
             = detail::CheckExtensionSupport{ extensions }( physical_device )
                   .and_then( detail::AppendSupportedExtensions{ extensions, optional_extensions } )
                   .and_then( detail::CheckFeatureSupport{ settings.device_features } )
                   .and_then( detail::CheckSurfaceSupport{ vk_surface } )
                   .and_then( BuildQueueFamilyMap{ settings.queue_flags, vk_surface } ) )
        {
            sorted_devices.emplace(
                physical_device,
                std::move( queue_families.value( ) ),
                std::move( extensions )
            );
        }
        else
        {
            // If no device is found, collect the errors and return them.
            error_messages.push_back( queue_families.error( ).error_message( ) );
        }
    }

    if ( sorted_devices.empty( ) )
    {
        return LTB_MAKE_UNEXPECTED_ERROR(
            "Failed to find a suitable GPU:\n{}",
            fmt::join( error_messages, "\n" )
        );
    }

    auto selected_device = sorted_devices.top( );

    if ( !settings.preferred_depth_formats.empty( ) )
    {
        LTB_CHECK(
            depth_image_format_,
            find_supported_format(
                selected_device.physical_device,
                settings.preferred_depth_formats,
                settings.preferred_depth_tiling,
                vk::FormatFeatureFlagBits::eDepthStencilAttachment
            )
        );
    }

    settings_              = std::move( settings );
    physical_device_       = selected_device.physical_device;
    extensions_            = std::move( selected_device.extensions );
    properties_            = physical_device_.getProperties( );
    queue_families_        = std::move( selected_device.queue_families );
    unique_queue_families_ = queue_families_ | std::views::values | std::ranges::to< std::set >( );

    spdlog::
        info( "Selected device {}", std::string( physical_device_.getProperties( ).deviceName.data( ) ) );

    return utils::success( );
}

auto PhysicalDevice::is_initialized( ) const -> bool
{
    return nullptr != physical_device_;
}

auto PhysicalDevice::get( ) const -> vk::PhysicalDevice const&
{
    return physical_device_;
}

auto PhysicalDevice::get( ) -> vk::PhysicalDevice&
{
    return physical_device_;
}

auto PhysicalDevice::properties( ) const -> vk::PhysicalDeviceProperties const&
{
    return properties_;
}

auto PhysicalDevice::identity( ) const -> DeviceIdentity
{
    auto const properties
        = physical_device_
              .getProperties2< vk::PhysicalDeviceProperties2, vk::PhysicalDeviceIDProperties >( );
    auto const& id_properties = properties.get< vk::PhysicalDeviceIDProperties >( );

    auto identity = DeviceIdentity{ };
    std::ranges::copy( id_properties.deviceUUID, identity.device_uuid.begin( ) );
    std::ranges::copy( id_properties.driverUUID, identity.driver_uuid.begin( ) );
    return identity;
}

auto PhysicalDevice::settings( ) const -> DeviceSettings const&
{
    return settings_;
}

auto PhysicalDevice::extensions( ) const -> std::vector< char const* > const&
{
    return extensions_;
}

auto PhysicalDevice::queue_families( ) const -> QueueFamilyMap const&
{
    return queue_families_;
}

auto PhysicalDevice::unique_queue_families( ) const -> std::set< QueueIndex > const&
{
    return unique_queue_families_;
}

auto PhysicalDevice::depth_image_format( ) const -> vk::Format
{
    return depth_image_format_;
}

auto PhysicalDevice::find_memory_type_index(
    std::vector< vk::MemoryRequirements > const& memory_requirements,
    vk::MemoryPropertyFlags const                memory_properties
) const -> utils::Result< uint32 >
{
    auto const device_memory_properties = this->get( ).getMemoryProperties( );

    for ( auto i = 0U; i < device_memory_properties.memoryTypeCount; ++i )
    {
        if ( std::ranges::all_of(
                 memory_requirements,
                 detail::IsSuitableMemoryType{
                     .index                    = i,
                     .device_memory_properties = device_memory_properties,
                     .properties               = memory_properties
                 }
             ) )
        {
            return i;
        }
    }

    return LTB_MAKE_UNEXPECTED_ERROR( "Failed to find suitable memory type" );
}

} // namespace ltb::vlk
