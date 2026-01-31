// ///////////////////////////////////////////////////////////////////////////////////////
// A Logan Thomas Barnes project
// ///////////////////////////////////////////////////////////////////////////////////////
#pragma once

// project
#include "ltb/utils/result.hpp"
#include "ltb/utils/types.hpp"
#include "ltb/vlk/fwd.hpp"
#include "ltb/vlk/vulkan.hpp"

namespace ltb::vlk
{

struct FramebufferSettings
{
    std::vector< vk::ImageView > attachments = { };
    vk::Extent2D                 extent      = { 0, 0 };
    uint32                       layers      = 1U;
};

class Framebuffer
{
public:
    Framebuffer( Device& device, RenderPass& render_pass );

    auto initialize( FramebufferSettings settings ) -> utils::Result< void >;

    [[nodiscard( "Const getter" )]]
    auto is_initialized( ) const -> bool;

    [[nodiscard( "Const getter" )]]
    auto get( ) const -> vk::Framebuffer const&;
    auto get( ) -> vk::Framebuffer&;

    [[nodiscard( "Const getter" )]]
    auto settings( ) const -> FramebufferSettings const&;

private:
    Device&     device_;
    RenderPass& render_pass_;

    FramebufferSettings   settings_    = { };
    vk::UniqueFramebuffer framebuffer_ = { };
};

} // namespace ltb::vlk
