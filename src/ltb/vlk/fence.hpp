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

struct FenceSettings
{
    vk::FenceCreateFlags flags = vk::FenceCreateFlagBits::eSignaled;
};

class Fence
{
public:
    explicit( false ) Fence( Device& device );

    auto initialize( ) -> utils::Result< void >;
    auto initialize( FenceSettings settings ) -> utils::Result< void >;

    [[nodiscard( "Const getter" )]]
    auto is_initialized( ) const -> bool;

    [[nodiscard( "Const getter" )]]
    auto get( ) const -> vk::Fence const&;
    auto get( ) -> vk::Fence&;

private:
    Device& device_;

    FenceSettings   settings_ = { };
    vk::UniqueFence fence_    = { };
};

} // namespace ltb::vlk
