// ///////////////////////////////////////////////////////////////////////////////////////
// A Logan Thomas Barnes project
// ///////////////////////////////////////////////////////////////////////////////////////
#include "ltb/vlk/detail/device_functors.hpp"

// project
#include "ltb/vlk/check.hpp"
#include "ltb/vlk/instance.hpp"

// external
#include <range/v3/algorithm/any_of.hpp>
#include <range/v3/algorithm/max.hpp>
#include <range/v3/range/conversion.hpp>
#include <range/v3/view/filter.hpp>
#include <range/v3/view/map.hpp>
#include <range/v3/view/remove_if.hpp>
#include <spdlog/spdlog.h>

// standard
#include <queue>
#include <set>

namespace ltb::vlk::detail
{
namespace
{

struct ExtensionNameMatches
{
    char const* const extension_name;

    auto operator( )( vk::ExtensionProperties const& extension ) const -> bool
    {
        return std::string( extension.extensionName.data( ) ) == extension_name;
    }
};

struct ContainsExtension
{
    std::vector< vk::ExtensionProperties > const& available_extensions;

    auto operator( )( char const* const extension_name ) const -> bool
    {
        return ranges::any_of( available_extensions, ExtensionNameMatches{ extension_name } );
    }
};

} // namespace

auto CheckExtensionSupport::operator( )( vk::PhysicalDevice const& physical_device ) const
    -> utils::Result< vk::PhysicalDevice >
{
    VK_CHECK(
        auto const available_extensions,
        physical_device.enumerateDeviceExtensionProperties( nullptr )
    );

    // Find any `extensions` that are not listed in `available_extensions`.
    if ( auto const unsupported_extensions
         = extensions | ranges::views::remove_if( ContainsExtension{ available_extensions } )
         | ranges::to< std::set >;
         !unsupported_extensions.empty( ) )
    {
        return LTB_MAKE_UNEXPECTED_ERROR(
            "Extensions not supported:\n{}",
            fmt::join( unsupported_extensions, "\n" )
        );
    }

    return physical_device;
}

auto AppendSupportedExtensions::operator( )( vk::PhysicalDevice const& physical_device ) const
    -> utils::Result< vk::PhysicalDevice >
{
    VK_CHECK(
        auto const available_extensions,
        physical_device.enumerateDeviceExtensionProperties( nullptr )
    );

    // Keep all `extensions` that are listed in `available_extensions`.
    auto const supported_extensions
        = optional_extensions | ranges::views::filter( ContainsExtension{ available_extensions } )
        | ranges::to< std::set >;

    extensions.insert(
        // Append supported extensions
        extensions.end( ),
        supported_extensions.begin( ),
        supported_extensions.end( )
    );

    return physical_device;
}

auto CheckFeatureSupport::operator( )( vk::PhysicalDevice const& physical_device ) const
    -> utils::Result< vk::PhysicalDevice >
{
    auto const supported_features = physical_device.getFeatures2( );
    auto const feature_count      = static_cast< uint32 >( features.size( ) );
    for ( auto i = 0U; i < feature_count; ++i )
    {
        if ( !( supported_features.features.*features[ i ] ) )
        {
            return LTB_MAKE_UNEXPECTED_ERROR( "feature[{}] not supported on this device", i );
        }
    }
    return physical_device;
}

auto CheckSurfaceSupport::operator( )( vk::PhysicalDevice const& physical_device ) const
    -> utils::Result< vk::PhysicalDevice >
{
    if ( !surface )
    {
        return physical_device;
    }

    VK_CHECK( auto const surface_formats, physical_device.getSurfaceFormatsKHR( surface ) );
    if ( surface_formats.empty( ) )
    {
        return LTB_MAKE_UNEXPECTED_ERROR( "Surface formats not suitable" );
    }

    VK_CHECK(
        auto const surface_present_modes,
        physical_device.getSurfacePresentModesKHR( surface )
    );
    if ( surface_present_modes.empty( ) )
    {
        return LTB_MAKE_UNEXPECTED_ERROR( "Surface present modes not suitable" );
    }

    return physical_device;
}

SuitablePhysicalDevice::SuitablePhysicalDevice(
    vk::PhysicalDevice const   potential_device,
    QueueFamilyMap             queue_family_map,
    std::vector< char const* > supported_extensions
)
    : physical_device( potential_device )
    , queue_families( std::move( queue_family_map ) )
    , extensions( std::move( supported_extensions ) )
{
    auto const properties = physical_device.getProperties( );
    spdlog::info( "Found device: {}", properties.deviceName );

    if ( vk::PhysicalDeviceType::eDiscreteGpu == properties.deviceType )
    {
        score += 1000U;
    }

    score += static_cast< uint32 >( supported_extensions.size( ) ) * 100U;

    // Maximum possible size of textures affects graphics quality
    score += properties.limits.maxImageDimension2D;
}

} // namespace ltb::vlk::detail
