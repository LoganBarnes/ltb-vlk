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

struct RenderPassSettings
{
    vk::Format color_attachment_format = vk::Format::eUndefined;
    vk::Format depth_attachment_format = vk::Format::eD32Sfloat;

    std::vector< vk::AttachmentDescription > attachments = {
        vk::AttachmentDescription{ }
            .setFormat( color_attachment_format )
            .setSamples( vk::SampleCountFlagBits::e1 )
            .setLoadOp( vk::AttachmentLoadOp::eClear )
            .setStoreOp( vk::AttachmentStoreOp::eStore )
            .setStencilLoadOp( vk::AttachmentLoadOp::eDontCare )
            .setStencilStoreOp( vk::AttachmentStoreOp::eDontCare )
            .setInitialLayout( vk::ImageLayout::eUndefined )
            .setFinalLayout( vk::ImageLayout::ePresentSrcKHR ),
        vk::AttachmentDescription{ }
            .setFormat( depth_attachment_format )
            .setSamples( vk::SampleCountFlagBits::e1 )
            .setLoadOp( vk::AttachmentLoadOp::eClear )
            .setStoreOp( vk::AttachmentStoreOp::eDontCare )
            .setStencilLoadOp( vk::AttachmentLoadOp::eDontCare )
            .setStencilStoreOp( vk::AttachmentStoreOp::eDontCare )
            .setInitialLayout( vk::ImageLayout::eUndefined )
            .setFinalLayout( vk::ImageLayout::eDepthStencilAttachmentOptimal ),
    };

    std::vector< vk::AttachmentReference > color_attachment_refs = {
        vk::AttachmentReference{ }.setAttachment( 0 ).setLayout(
            vk::ImageLayout::eColorAttachmentOptimal
        ),
    };

    vk::AttachmentReference depth_attachment_ref
        = vk::AttachmentReference{ }.setAttachment( 1 ).setLayout(
            vk::ImageLayout::eDepthStencilAttachmentOptimal
        );

    std::vector< vk::SubpassDescription > subpasses = {
        vk::SubpassDescription{ }
            .setPipelineBindPoint( vk::PipelineBindPoint::eGraphics )
            .setColorAttachments( color_attachment_refs )
            .setPDepthStencilAttachment( &depth_attachment_ref ),
    };

    std::vector< vk::SubpassDependency > dependencies = {
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
};

auto render_pass_settings_3d( vk::Format color_format, vk::Format depth_format )
    -> RenderPassSettings;

auto render_pass_settings_2d( vk::Format color_format ) -> RenderPassSettings;

class RenderPass
{
public:
    explicit( false ) RenderPass( Device& device );

    auto initialize( RenderPassSettings settings ) -> utils::Result< void >;
    auto initialize( RenderPassSettings settings, Swapchain const* swapchain )
        -> utils::Result< void >;

    [[nodiscard( "Const getter" )]]
    auto is_initialized( ) const -> bool;

    [[nodiscard( "Const getter" )]]
    auto get( ) const -> vk::RenderPass const&;
    auto get( ) -> vk::RenderPass&;

    auto reset( ) -> void;

    [[nodiscard( "Const getter" )]]
    auto settings( ) const -> RenderPassSettings const&;

private:
    Device& device_;

    RenderPassSettings   settings_    = { };
    vk::UniqueRenderPass render_pass_ = { };
};

} // namespace ltb::vlk
