// ///////////////////////////////////////////////////////////////////////////////////////
// A Logan Thomas Barnes project
// ///////////////////////////////////////////////////////////////////////////////////////
#include "ltb/vlk/device.hpp"

// project
#include "ltb/utils/types.hpp"
#include "ltb/vlk/check.hpp"
#include "ltb/vlk/physical_device.hpp"

// external
#include <spdlog/spdlog.h>

// standard
#include <algorithm>
#include <ranges>
#include <string_view>

namespace ltb::vlk
{
namespace
{

struct CreateDeviceQueueCreateInfo
{
    std::vector< float32 > const& queue_priorities;

    auto operator( )( uint32 const queue_family_index ) const
    {
        return vk::DeviceQueueCreateInfo{ }
            .setQueueFamilyIndex( queue_family_index )
            .setQueuePriorities( queue_priorities );
    }
};

struct MakeQueuePair
{
    vk::UniqueDevice const& device;

    auto operator( )( auto const& type_and_index ) const
    {
        auto const [ type, family_index ] = type_and_index;
        return std::make_pair( type, device->getQueue( family_index, 0 ) );
    }
};

} // namespace

Device::Device( PhysicalDevice& physical_device )
    : physical_device_( physical_device )
{
}

auto Device::initialize( ) -> utils::Result< void >
{
    if ( this->is_initialized( ) )
    {
        return utils::success( );
    }
    LTB_CHECK_VALID( physical_device_.is_initialized( ) );

    auto const& settings = physical_device_.settings( );

    auto const queue_priorities = std::vector{ 1.0f };

    auto const queue_create_infos
        = physical_device_.unique_queue_families( )
        | std::views::transform( CreateDeviceQueueCreateInfo{ queue_priorities } )
        | std::ranges::to< std::vector >( );

    auto enable_fifo_latest_ready = vk::PhysicalDevicePresentModeFifoLatestReadyFeaturesKHR{ true };

    // Compare names by value; the extension list may hold different copies of the string.
    auto const fifo_latest_ready_extension_enabled = std::ranges::
        any_of( physical_device_.extensions( ), []( char const* const extension ) -> bool {
            return std::string_view{ extension }
                == VK_KHR_PRESENT_MODE_FIFO_LATEST_READY_EXTENSION_NAME;
        } );

    // The extension being available doesn't mean the GPU supports the feature.
    auto fifo_latest_ready_supported = false;
    if ( fifo_latest_ready_extension_enabled )
    {
        auto const features = physical_device_.get( )
                                  .getFeatures2<
                                      vk::PhysicalDeviceFeatures2,
                                      vk::PhysicalDevicePresentModeFifoLatestReadyFeaturesKHR >( );
        fifo_latest_ready_supported
            = ( vk::True
                == features.get< vk::PhysicalDevicePresentModeFifoLatestReadyFeaturesKHR >( )
                       .presentModeFifoLatestReady );
    }

    auto  device_features_2 = vk::PhysicalDeviceFeatures2{ };
    auto& device_features   = device_features_2.features;

    for ( auto const feature : settings.device_features )
    {
        ( device_features.*feature ) = true;
    }

    if ( fifo_latest_ready_supported )
    {
        device_features_2.pNext = &enable_fifo_latest_ready;
    }

    auto const create_info = vk::DeviceCreateInfo{ }
                                 .setQueueCreateInfos( queue_create_infos )
                                 .setPEnabledExtensionNames( physical_device_.extensions( ) )
                                 .setPNext( device_features_2 );

    VK_CHECK( auto device, physical_device_.get( ).createDeviceUnique( create_info ) );
    spdlog::debug( "vk::createDeviceUnique()" );

    device_                    = std::move( device );
    fifo_latest_ready_enabled_ = fifo_latest_ready_supported;
    queues_                    = physical_device_.queue_families( )
            | std::ranges::views::transform( MakeQueuePair{ device_ } )
            | std::ranges::to< QueueMap >( );

    return utils::success( );
}

auto Device::is_initialized( ) const -> bool
{
    return nullptr != device_.get( );
}

auto Device::get( ) const -> vk::Device const&
{
    return device_.get( );
}

auto Device::get( ) -> vk::Device&
{
    return device_.get( );
}

auto Device::physical_device( ) const -> PhysicalDevice const&
{
    return physical_device_;
}

auto Device::queues( ) const -> QueueMap const&
{
    return queues_;
}

auto Device::fifo_latest_ready_enabled( ) const -> bool
{
    return fifo_latest_ready_enabled_;
}

} // namespace ltb::vlk
