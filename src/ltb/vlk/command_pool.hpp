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

struct CommandPoolSettings
{
    QueueType                  queue_type = QueueType::Unknown;
    vk::CommandPoolCreateFlags flags      = vk::CommandPoolCreateFlagBits::eResetCommandBuffer;
};

class CommandPool
{
public:
    CommandPool( Device& device, PhysicalDevice& physical_device );

    auto initialize( ) -> utils::Result< void >;
    auto initialize( CommandPoolSettings settings ) -> utils::Result< void >;

    [[nodiscard( "Const getter" )]]
    auto is_initialized( ) const -> bool;

    [[nodiscard( "Const getter" )]]
    auto get( ) const -> vk::CommandPool const&;
    auto get( ) -> vk::CommandPool&;

private:
    Device&         device_;
    PhysicalDevice& physical_device_;

    CommandPoolSettings   settings_     = { };
    vk::UniqueCommandPool command_pool_ = { };
};

} // namespace ltb::vlk
