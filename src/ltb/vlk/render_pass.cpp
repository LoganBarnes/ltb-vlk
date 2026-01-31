// ///////////////////////////////////////////////////////////////////////////////////////
// A Logan Thomas Barnes project
// ///////////////////////////////////////////////////////////////////////////////////////
#include "ltb/vlk/render_pass.hpp"

// project
#include "ltb/vlk/check.hpp"
#include "ltb/vlk/device.hpp"
#include "ltb/vlk/swapchain.hpp"

// external
#include <spdlog/spdlog.h>

// standard
#include <ranges>

namespace ltb::vlk
{

auto render_pass_settings_3d( vk::Format color_format, vk::Format depth_format )
    -> RenderPassSettings
{
    auto settings = RenderPassSettings{ };

    settings.color_attachment_format = color_format;
    settings.depth_attachment_format = depth_format;

    settings.attachments = {
        vk::AttachmentDescription{ }
            .setFormat( color_format )
            .setSamples( vk::SampleCountFlagBits::e1 )
            .setLoadOp( vk::AttachmentLoadOp::eClear )
            .setStoreOp( vk::AttachmentStoreOp::eStore )
            .setStencilLoadOp( vk::AttachmentLoadOp::eDontCare )
            .setStencilStoreOp( vk::AttachmentStoreOp::eDontCare )
            .setInitialLayout( vk::ImageLayout::eUndefined )
            .setFinalLayout( vk::ImageLayout::ePresentSrcKHR ),
        vk::AttachmentDescription{ }
            .setFormat( depth_format )
            .setSamples( vk::SampleCountFlagBits::e1 )
            .setLoadOp( vk::AttachmentLoadOp::eClear )
            .setStoreOp( vk::AttachmentStoreOp::eDontCare )
            .setStencilLoadOp( vk::AttachmentLoadOp::eDontCare )
            .setStencilStoreOp( vk::AttachmentStoreOp::eDontCare )
            .setInitialLayout( vk::ImageLayout::eUndefined )
            .setFinalLayout( vk::ImageLayout::eDepthStencilAttachmentOptimal ),
    };

    settings.color_attachment_refs = {
        vk::AttachmentReference{ }.setAttachment( 0 ).setLayout(
            vk::ImageLayout::eColorAttachmentOptimal
        ),
    };

    settings.depth_attachment_ref = vk::AttachmentReference{ }.setAttachment( 1 ).setLayout(
        vk::ImageLayout::eDepthStencilAttachmentOptimal
    );

    settings.subpasses = {
        vk::SubpassDescription{ }
            .setPipelineBindPoint( vk::PipelineBindPoint::eGraphics )
            .setColorAttachments( settings.color_attachment_refs )
            .setPDepthStencilAttachment( &settings.depth_attachment_ref ),
    };

    settings.dependencies = {
        vk::SubpassDependency{ }
            .setSrcSubpass( VK_SUBPASS_EXTERNAL )
            .setDstSubpass( 0U )
            .setSrcStageMask(
                vk::PipelineStageFlagBits::eColorAttachmentOutput
                | vk::PipelineStageFlagBits::eLateFragmentTests
            )
            .setSrcAccessMask( vk::AccessFlagBits::eDepthStencilAttachmentWrite )
            .setDstStageMask(
                vk::PipelineStageFlagBits::eColorAttachmentOutput
                | vk::PipelineStageFlagBits::eEarlyFragmentTests
            )
            .setDstAccessMask(
                vk::AccessFlagBits::eColorAttachmentWrite
                | vk::AccessFlagBits::eDepthStencilAttachmentWrite
            ),
    };

    return settings;
}

auto render_pass_settings_2d( vk::Format const color_format ) -> RenderPassSettings
{
    auto settings = RenderPassSettings{ };

    settings.color_attachment_format = color_format;
    settings.depth_attachment_format = vk::Format::eUndefined;

    settings.attachments = {
        vk::AttachmentDescription{ }
            .setFormat( color_format )
            .setSamples( vk::SampleCountFlagBits::e1 )
            .setLoadOp( vk::AttachmentLoadOp::eClear )
            .setStoreOp( vk::AttachmentStoreOp::eStore )
            .setStencilLoadOp( vk::AttachmentLoadOp::eDontCare )
            .setStencilStoreOp( vk::AttachmentStoreOp::eDontCare )
            .setInitialLayout( vk::ImageLayout::eUndefined )
            .setFinalLayout( vk::ImageLayout::ePresentSrcKHR ),
    };

    settings.color_attachment_refs = {
        vk::AttachmentReference{ }.setAttachment( 0 ).setLayout(
            vk::ImageLayout::eColorAttachmentOptimal
        ),
    };

    settings.subpasses = {
        vk::SubpassDescription{ }
            .setPipelineBindPoint( vk::PipelineBindPoint::eGraphics )
            .setColorAttachments( settings.color_attachment_refs ),
    };

    settings.dependencies = {
        vk::SubpassDependency{ }
            .setSrcSubpass( VK_SUBPASS_EXTERNAL )
            .setDstSubpass( 0U )
            .setSrcStageMask( vk::PipelineStageFlagBits::eColorAttachmentOutput )
            .setSrcAccessMask( vk::AccessFlagBits::eNone )
            .setDstStageMask( vk::PipelineStageFlagBits::eColorAttachmentOutput )
            .setDstAccessMask( vk::AccessFlagBits::eColorAttachmentWrite ),
    };
    return settings;
}

RenderPass::RenderPass( Device& device )
    : device_( device )
{
}

auto RenderPass::initialize( RenderPassSettings settings ) -> utils::Result< void >
{
    return this->initialize( std::move( settings ), nullptr );
}

auto RenderPass::initialize( RenderPassSettings settings, Swapchain const* const swapchain )
    -> utils::Result< void >
{
    if ( this->is_initialized( ) )
    {
        return utils::success( );
    }
    LTB_CHECK_VALID( device_.is_initialized( ) );

    for ( auto& attachment : settings.attachments )
    {
        if ( vk::Format::eUndefined == attachment.format )
        {
            if ( ( vk::ImageLayout::ePresentSrcKHR == attachment.finalLayout )
                 && ( nullptr != swapchain ) )
            {
                attachment.format = swapchain->image_format( );
            }
            else
            {
                return LTB_MAKE_UNEXPECTED_ERROR( "initialize(): attachment has undefined format" );
            }
        }
    }

    auto const render_pass_info = vk::RenderPassCreateInfo{ }
                                      .setAttachments( settings.attachments )
                                      .setSubpasses( settings.subpasses )
                                      .setDependencies( settings.dependencies );

    VK_CHECK( auto render_pass, device_.get( ).createRenderPassUnique( render_pass_info ) );
    spdlog::debug( "vk::createRenderPassUnique()" );

    settings_    = std::move( settings );
    render_pass_ = std::move( render_pass );

    return utils::success( );
}

auto RenderPass::is_initialized( ) const -> bool
{
    return nullptr != render_pass_.get( );
}

auto RenderPass::get( ) const -> vk::RenderPass const&
{
    return render_pass_.get( );
}

auto RenderPass::get( ) -> vk::RenderPass&
{
    return render_pass_.get( );
}

auto RenderPass::reset( ) -> void
{
    render_pass_.reset( );
    settings_ = { };
}

auto RenderPass::settings( ) const -> RenderPassSettings const&
{
    return settings_;
}

} // namespace ltb::vlk
