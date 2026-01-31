// ///////////////////////////////////////////////////////////////////////////////////////
// A Logan Thomas Barnes project
// ///////////////////////////////////////////////////////////////////////////////////////
#include "ltb/vlk/framebuffer.hpp"

// project
#include "ltb/vlk/check.hpp"
#include "ltb/vlk/device.hpp"
#include "ltb/vlk/render_pass.hpp"

// external
#include <spdlog/spdlog.h>

namespace ltb::vlk
{

Framebuffer::Framebuffer( Device& device, RenderPass& render_pass )
    : device_( device )
    , render_pass_( render_pass )
{
}

auto Framebuffer::initialize( FramebufferSettings settings ) -> utils::Result< void >
{
    if ( this->is_initialized( ) )
    {
        return utils::success( );
    }
    LTB_CHECK_VALID( device_.is_initialized( ) );
    LTB_CHECK_VALID( render_pass_.is_initialized( ) );

    auto const framebuffer_info = vk::FramebufferCreateInfo{ }
                                      .setRenderPass( render_pass_.get( ) )
                                      .setAttachments( settings.attachments )
                                      .setWidth( settings.extent.width )
                                      .setHeight( settings.extent.height )
                                      .setLayers( settings.layers );

    VK_CHECK( auto framebuffer, device_.get( ).createFramebufferUnique( framebuffer_info ) );
    spdlog::debug( "vk::createFramebufferUnique()" );

    settings_    = std::move( settings );
    framebuffer_ = std::move( framebuffer );

    return utils::success( );
}

auto Framebuffer::is_initialized( ) const -> bool
{
    return nullptr != framebuffer_.get( );
}

auto Framebuffer::get( ) const -> vk::Framebuffer const&
{
    return framebuffer_.get( );
}

auto Framebuffer::get( ) -> vk::Framebuffer&
{
    return framebuffer_.get( );
}

auto Framebuffer::settings( ) const -> FramebufferSettings const&
{
    return settings_;
}

} // namespace ltb::vlk
