// ///////////////////////////////////////////////////////////////////////////////////////
// A Logan Thomas Barnes project
// ///////////////////////////////////////////////////////////////////////////////////////
#include "ltb/vlk/detail/device_functors.hpp"

// project
#include "ltb/vlk/check.hpp"
#include "ltb/vlk/instance.hpp"

// external
#include <spdlog/fmt/ranges.h>
#include <spdlog/spdlog.h>

// standard
#include <queue>
#include <ranges>
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
        return std::ranges::any_of( available_extensions, ExtensionNameMatches{ extension_name } );
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
         = extensions
         | std::views::filter( std::not_fn( ContainsExtension{ available_extensions } ) )
         | std::ranges::to< std::set >( );
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
        = optional_extensions | std::views::filter( ContainsExtension{ available_extensions } )
        | std::ranges::to< std::set >( );

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
    spdlog::info( "Found device: {}", std::string{ properties.deviceName } );

    score = score_physical_device( properties, static_cast< uint32 >( extensions.size( ) ) );
}

auto score_physical_device(
    vk::PhysicalDeviceProperties const& properties,
    uint32 const                        supported_extension_count
) -> uint64
{
    auto device_type_rank = uint64{ 0U };
    switch ( properties.deviceType )
    {
        using enum vk::PhysicalDeviceType;
        case eDiscreteGpu:
            device_type_rank = 4U;
            break;
        case eIntegratedGpu:
            device_type_rank = 3U;
            break;
        case eVirtualGpu:
            device_type_rank = 2U;
            break;
        case eCpu:
            device_type_rank = 1U;
            break;
        default:
            break;
    }

    // Each tier is weighted so it can never be outweighed by the tiers below it.
    // (maxImageDimension2D and the extension count are both far below 2^32.)
    constexpr auto extension_weight   = uint64{ 1U } << 32U;
    constexpr auto device_type_weight = uint64{ 1U } << 48U;

    return ( device_type_rank * device_type_weight )
         + ( uint64{ supported_extension_count } * extension_weight )
         + uint64{ properties.limits.maxImageDimension2D };
}

} // namespace ltb::vlk::detail
