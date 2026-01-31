// ///////////////////////////////////////////////////////////////////////////////////////
// A Logan Thomas Barnes project
// ///////////////////////////////////////////////////////////////////////////////////////
#include "ltb/vlk/device.hpp"

// project
#include "ltb/utils/types.hpp"
#include "ltb/vlk/check.hpp"
#include "ltb/vlk/physical_device.hpp"

// external
#include <range/v3/range/conversion.hpp>
#include <range/v3/view/transform.hpp>
#include <spdlog/spdlog.h>

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
        | ranges::views::transform( CreateDeviceQueueCreateInfo{ queue_priorities } )
        | ranges::to< std::vector >( );

    auto enable_fifo_latest_ready = vk::PhysicalDevicePresentModeFifoLatestReadyFeaturesKHR{ true };

    auto  device_features_2 = vk::PhysicalDeviceFeatures2{ };
    auto& device_features   = device_features_2.features;

    for ( auto const feature : settings.device_features )
    {
        ( device_features.*feature ) = true;
    }

    if ( std::ranges::find(
             physical_device_.extensions( ),
             VK_KHR_PRESENT_MODE_FIFO_LATEST_READY_EXTENSION_NAME
         )
         != physical_device_.extensions( ).end( ) )
    {
        device_features_2.pNext = &enable_fifo_latest_ready;
    }

    auto const create_info = vk::DeviceCreateInfo{ }
                                 .setQueueCreateInfos( queue_create_infos )
                                 .setPEnabledExtensionNames( physical_device_.extensions( ) )
                                 .setPNext( device_features_2 );

    VK_CHECK( auto device, physical_device_.get( ).createDeviceUnique( create_info ) );
    spdlog::debug( "vk::createDeviceUnique()" );

    device_ = std::move( device );
    queues_ = physical_device_.queue_families( )
            | ranges::views::transform( MakeQueuePair{ device_ } ) | ranges::to< QueueMap >( );

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

} // namespace ltb::vlk
