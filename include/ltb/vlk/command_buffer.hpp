// ///////////////////////////////////////////////////////////////////////////////////////
// A Logan Thomas Barnes project
// ///////////////////////////////////////////////////////////////////////////////////////
#pragma once

// project
#include "ltb/utils/result.hpp"
#include "ltb/vlk/fwd.hpp"
#include "ltb/vlk/vulkan.hpp"

namespace ltb::vlk
{

struct CommandBufferSettings
{
    vk::CommandBufferLevel level = vk::CommandBufferLevel::ePrimary;
};

class CommandBuffer
{
public:
    CommandBuffer( Device& device, CommandPool& command_pool );

    auto initialize( ) -> utils::Result< void >;
    auto initialize( CommandBufferSettings settings ) -> utils::Result< void >;

    [[nodiscard( "Const getter" )]]
    auto is_initialized( ) const -> bool;

    [[nodiscard( "Const getter" )]]
    auto get( ) const -> vk::CommandBuffer const&;
    auto get( ) -> vk::CommandBuffer&;

private:
    Device&      device_;
    CommandPool& command_pool_;

    CommandBufferSettings   settings_       = { };
    vk::UniqueCommandBuffer command_buffer_ = { };
};

} // namespace ltb::vlk
