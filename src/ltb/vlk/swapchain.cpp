// ///////////////////////////////////////////////////////////////////////////////////////
// A Logan Thomas Barnes project
// ///////////////////////////////////////////////////////////////////////////////////////
#include "ltb/vlk/swapchain.hpp"

// project
#include "ltb/utils/container_utils.hpp"
#include "ltb/vlk/check.hpp"
#include "ltb/vlk/device.hpp"
#include "ltb/vlk/instance.hpp"
#include "ltb/vlk/physical_device.hpp"
#include "ltb/vlk/surface.hpp"

// external
#include <range/v3/range/conversion.hpp>
#include <range/v3/view/map.hpp>
#include <spdlog/spdlog.h>

namespace ltb::vlk
{

Swapchain::Swapchain(
    Instance&       instance,
    Surface&        surface,
    PhysicalDevice& physical_device,
    Device&         device
)
    : instance_( instance )
    , surface_( surface )
    , physical_device_( physical_device )
    , device_( device )
{
}

auto Swapchain::initialize( SwapchainSettings settings ) -> utils::Result< void >
{
    return initialize( std::move( settings ), Reinitialize::OnlyIfNotInitialized );
}

auto Swapchain::initialize( SwapchainSettings settings, Reinitialize const reinitialize )
    -> utils::Result< void >
{
    if ( this->is_initialized( ) && ( Reinitialize::OnlyIfNotInitialized == reinitialize ) )
    {
        return utils::success( );
    }
    LTB_CHECK_VALID( instance_.is_initialized( ) );
    LTB_CHECK_VALID( surface_.is_initialized( ) );
    LTB_CHECK_VALID( physical_device_.is_initialized( ) );
    LTB_CHECK_VALID( device_.is_initialized( ) );

    image_format_ = { };
    images_.clear( );
    min_image_count_ = 0U;

    LTB_CHECK_VALID( 0U != settings.extent.width );
    LTB_CHECK_VALID( 0U != settings.extent.height );

    VK_CHECK(
        auto const surface_formats,
        physical_device_.get( ).getSurfaceFormatsKHR( surface_.get( ) )
    );
    LTB_CHECK(
        auto const surface_format,
        utils::find_first_available( settings.preferred_surface_formats, surface_formats )
    );

    VK_CHECK(
        auto const surface_present_modes,
        physical_device_.get( ).getSurfacePresentModesKHR( surface_.get( ) )
    );
    LTB_CHECK(
        auto const present_mode,
        utils::find_first_available( settings.preferred_present_modes, surface_present_modes )
    );
    spdlog::info( "Present Mode: {}", vk::to_string( present_mode ) );

    VK_CHECK(
        auto const surface_capabilities,
        physical_device_.get( ).getSurfaceCapabilitiesKHR( surface_.get( ) )
    );
    settings.extent = vk::Extent2D{
        std::clamp(
            settings.extent.width,
            surface_capabilities.minImageExtent.width,
            surface_capabilities.maxImageExtent.width
        ),
        std::clamp(
            settings.extent.height,
            surface_capabilities.minImageExtent.height,
            surface_capabilities.maxImageExtent.height
        ),
    };

    // Attempt triple buffering
    auto min_image_count = std::max( surface_capabilities.minImageCount, 3U );

    // Don't exceed the max (zero means no maximum).
    if ( ( surface_capabilities.maxImageCount > 0U )
         && ( min_image_count > surface_capabilities.maxImageCount ) )
    {
        min_image_count = surface_capabilities.maxImageCount;
    }
    spdlog::info( "Min Image Count: {}", min_image_count );

    auto const queue_families = physical_device_.queue_families( );
    LTB_CHECK_VALID( queue_families.contains( vlk::QueueType::Surface ) );

    auto image_sharing_mode = vk::SharingMode::eExclusive;
    if ( physical_device_.unique_queue_families( ).size( ) > 1U )
    {
        image_sharing_mode = vk::SharingMode::eConcurrent;
    }
    spdlog::trace( "Swapchain image sharing: {}", to_string( image_sharing_mode ) );

    auto const queue_family_indices
        = queue_families | ranges::views::values | ranges::to< std::vector >( );

    auto const create_info = vk::SwapchainCreateInfoKHR{ }
                                 .setSurface( surface_.get( ) )
                                 .setImageArrayLayers( 1U )
                                 .setImageUsage( vk::ImageUsageFlagBits::eColorAttachment )
                                 .setCompositeAlpha( vk::CompositeAlphaFlagBitsKHR::eOpaque )
                                 .setClipped( true )
                                 .setImageFormat( surface_format.format )
                                 .setImageColorSpace( surface_format.colorSpace )
                                 .setPresentMode( present_mode )
                                 .setMinImageCount( min_image_count )
                                 .setImageExtent( settings.extent )
                                 .setPreTransform( surface_capabilities.currentTransform )
                                 .setImageSharingMode( image_sharing_mode )
                                 .setQueueFamilyIndices( queue_family_indices )
                                 .setOldSwapchain( swapchain_.get( ) );

    VK_CHECK( auto swapchain, device_.get( ).createSwapchainKHRUnique( create_info ) );
    spdlog::debug( "vk::createSwapchainKHRUnique()" );

    VK_CHECK( auto images, device_.get( ).getSwapchainImagesKHR( swapchain.get( ) ) );
    spdlog::debug( "vk::getSwapchainImagesKHR()" );

    settings_        = std::move( settings );
    swapchain_       = std::move( swapchain );
    min_image_count_ = min_image_count;
    images_          = std::move( images );
    image_format_    = surface_format.format;

    return utils::success( );
}

auto Swapchain::is_initialized( ) const -> bool
{
    return nullptr != swapchain_.get( );
}

auto Swapchain::get( ) const -> vk::SwapchainKHR const&
{
    return swapchain_.get( );
}

auto Swapchain::get( ) -> vk::SwapchainKHR&
{
    return swapchain_.get( );
}

auto Swapchain::settings( ) const -> SwapchainSettings const&
{
    return settings_;
}

auto Swapchain::min_image_count( ) const -> uint32
{
    return min_image_count_;
}

auto Swapchain::images( ) const -> std::vector< vk::Image > const&
{
    return images_;
}

auto Swapchain::images( ) -> std::vector< vk::Image >&
{
    return images_;
}

auto Swapchain::image_format( ) const -> vk::Format
{
    return image_format_;
}

} // namespace ltb::vlk
