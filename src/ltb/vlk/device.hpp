// ///////////////////////////////////////////////////////////////////////////////////////
// A Logan Thomas Barnes project
// ///////////////////////////////////////////////////////////////////////////////////////
#pragma once

// project
#include "ltb/utils/result.hpp"
#include "ltb/vlk/fwd.hpp"
#include "ltb/vlk/queue_types.hpp"
#include "ltb/vlk/vulkan.hpp"

namespace ltb::vlk
{

class Device
{
public:
    explicit( false ) Device( PhysicalDevice& physical_device );

    auto initialize( ) -> utils::Result< void >;

    [[nodiscard( "Const getter" )]]
    auto is_initialized( ) const -> bool;

    [[nodiscard( "Const getter" )]]
    auto get( ) const -> vk::Device const&;
    auto get( ) -> vk::Device&;

    [[nodiscard( "Const getter" )]]
    auto physical_device( ) const -> PhysicalDevice const&;

    [[nodiscard( "Const getter" )]]
    auto queues( ) const -> QueueMap const&;

private:
    PhysicalDevice& physical_device_;

    vk::UniqueDevice device_ = { };
    QueueMap         queues_ = { };
};

} // namespace ltb::vlk
