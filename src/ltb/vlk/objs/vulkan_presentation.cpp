// ///////////////////////////////////////////////////////////////////////////////////////
// A Logan Thomas Barnes project
// ///////////////////////////////////////////////////////////////////////////////////////
#include "ltb/vlk/objs/vulkan_presentation.hpp"

// project
#include "ltb/vlk/check.hpp"
#include "ltb/vlk/device.hpp"
#include "ltb/vlk/device_memory_utils.hpp"
#include "ltb/vlk/graphics_pipeline.hpp"
#include "ltb/vlk/physical_device.hpp"
#include "ltb/vlk/surface.hpp"

// external
#include <spdlog/spdlog.h>

namespace ltb::vlk::objs
{

VulkanPresentation::VulkanPresentation( VulkanGpu& gpu )
    : gpu_( gpu )
{
}

auto VulkanPresentation::initialize( VulkanPresentationSettings settings ) -> utils::Result< void >
{
    if ( this->is_initialized( ) )
    {
        return utils::success( );
    }
    LTB_CHECK_VALID( gpu_.is_initialized( ) );

    if ( ExtentMode::FromSurface == settings.extent_mode )
    {
        settings.swapchain.extent = gpu_.surface( ).framebuffer_size( );
    }

    LTB_CHECK( this->initialize_swapchain(
        std::move( settings.swapchain ),
        std::move( settings.render_pass )
    ) );

    initialized_ = true;

    return utils::success( );
}

auto VulkanPresentation::rebuild( VulkanPresentationRebuildSettings settings )
    -> utils::Result< void >
{
    if ( !this->is_initialized( ) )
    {
        return LTB_MAKE_UNEXPECTED_ERROR( "VulkanPresentation::rebuild(): not initialized" );
    }

    if ( ExtentMode::FromSurface == settings.extent_mode )
    {
        settings.swapchain.extent = gpu_.surface( ).framebuffer_size( );
    }

    if ( ( Rebuild::IfSizeChanged == settings.rebuild )
         && ( settings.swapchain.extent == swapchain_.settings( ).extent ) )
    {
        return utils::success( );
    }

    VK_CHECK( gpu_.device( ).get( ).waitIdle( ) );

    spdlog::debug( "{}", __FUNCTION__ );

    return this->initialize_swapchain( std::move( settings.swapchain ), render_pass_.settings( ) );
}

auto VulkanPresentation::is_initialized( ) const -> bool
{
    return initialized_;
}

auto VulkanPresentation::begin_render_pass( BeginRenderPassSettings const& settings )
    -> utils::Result< void >
{
    LTB_CHECK_VALID( settings.image_index < framebuffers_.size( ) );
    auto& framebuffer = framebuffers_[ settings.image_index ];

    auto clear_values = std::vector{
        vk::ClearValue{ }.setColor( {
            settings.color_clear_value.r,
            settings.color_clear_value.g,
            settings.color_clear_value.b,
            settings.color_clear_value.a,
        } ),
    };
    if ( depth_image_view_.is_initialized( ) )
    {
        clear_values.push_back(
            vk::ClearValue{ }.setDepthStencil( {
                settings.depth_clear_value,
                0U,
            } )
        );
    }

    auto const default_render_area = vk::Rect2D{ }.setExtent( swapchain_.settings( ).extent );
    auto const render_area         = settings.render_area.value_or( default_render_area );

    auto const render_pass_info = vk::RenderPassBeginInfo{ }
                                      .setRenderPass( render_pass_.get( ) )
                                      .setFramebuffer( framebuffer.get( ) )
                                      .setRenderArea( render_area )
                                      .setClearValues( clear_values );
    settings.command_buffer.beginRenderPass( render_pass_info, vk::SubpassContents::eInline );

    auto const default_viewport
        = vk::Viewport{ }
              .setX( 0.0F )
              .setY( 0.0F )
              .setWidth( static_cast< float32 >( swapchain_.settings( ).extent.width ) )
              .setHeight( static_cast< float32 >( swapchain_.settings( ).extent.height ) )
              .setMinDepth( 0.0F )
              .setMaxDepth( 1.0F );

    auto const     viewport             = settings.viewport.value_or( default_viewport );
    auto const     viewports            = std::vector{ viewport };
    constexpr auto first_viewport_index = 0UL;
    settings.command_buffer.setViewport( first_viewport_index, viewports );

    auto const default_scissor
        = vk::Rect2D{ }.setOffset( { 0, 0 } ).setExtent( swapchain_.settings( ).extent );

    auto const     scissor             = settings.scissor.value_or( default_scissor );
    auto const     scissors            = std::vector{ scissor };
    constexpr auto first_scissor_index = 0UL;
    settings.command_buffer.setScissor( first_scissor_index, scissors );

    return utils::success( );
}

auto VulkanPresentation::swapchain( ) const -> Swapchain const&
{
    return swapchain_;
}

auto VulkanPresentation::swapchain( ) -> Swapchain&
{
    return swapchain_;
}

auto VulkanPresentation::swapchain_image_views( ) const -> std::vector< ImageView > const&
{
    return swapchain_image_views_;
}

auto VulkanPresentation::swapchain_image_views( ) -> std::vector< ImageView >&
{
    return swapchain_image_views_;
}

auto VulkanPresentation::depth_image( ) const -> Image const&
{
    return d_image_.image( );
}

auto VulkanPresentation::depth_image( ) -> Image&
{
    return d_image_.image( );
}

auto VulkanPresentation::depth_memory( ) const -> DeviceMemory const&
{
    return d_image_.memory( );
}

auto VulkanPresentation::depth_memory( ) -> DeviceMemory&
{
    return d_image_.memory( );
}

auto VulkanPresentation::depth_image_view( ) const -> ImageView const&
{
    return depth_image_view_;
}

auto VulkanPresentation::depth_image_view( ) -> ImageView&
{
    return depth_image_view_;
}

auto VulkanPresentation::render_pass( ) const -> RenderPass const&
{
    return render_pass_;
}

auto VulkanPresentation::render_pass( ) -> RenderPass&
{
    return render_pass_;
}

auto VulkanPresentation::framebuffers( ) const -> std::vector< Framebuffer > const&
{
    return framebuffers_;
}

auto VulkanPresentation::framebuffers( ) -> std::vector< Framebuffer >&
{
    return framebuffers_;
}

auto VulkanPresentation::initialize_swapchain(
    SwapchainSettings  swapchain_settings,
    RenderPassSettings render_pass_settings
) -> utils::Result< void >
{
    spdlog::debug(
        "{}: {} x {}",
        __FUNCTION__,
        swapchain_settings.extent.width,
        swapchain_settings.extent.height
    );

    // No need to reset the render pass here.
    // It can persist across swapchain recreations.
    depth_image_view_.reset( );
    d_image_.reset( );
    framebuffers_.clear( );
    swapchain_image_views_.clear( );

    auto const extent = swapchain_settings.extent;

    LTB_CHECK( swapchain_.initialize( std::move( swapchain_settings ), Reinitialize::Always ) );

    swapchain_image_views_.reserve( swapchain_.images( ).size( ) );
    for ( auto const& image : swapchain_.images( ) )
    {
        LTB_CHECK( swapchain_image_views_.emplace_back( gpu_.device( ) )
                       .initialize( {
                           .image  = image,
                           .format = swapchain_.image_format( ),
                       } ) );
    }
    LTB_CHECK_VALID( swapchain_.images( ).size( ) == swapchain_image_views_.size( ) );

    if ( vk::Format::eUndefined != render_pass_settings.depth_attachment_format )
    {
        auto image = ImageSettings{
            .extent = { extent.width, extent.height, 1 },
            .format = render_pass_settings.depth_attachment_format,
            .usage  = vk::ImageUsageFlagBits::eDepthStencilAttachment,
        };
        LTB_CHECK( d_image_.initialize( {
            .image             = std::move( image ),
            .memory_properties = vk::MemoryPropertyFlagBits::eDeviceLocal,
        } ) );

        LTB_CHECK( depth_image_view_.initialize( {
            .image       = d_image_.image( ).get( ),
            .format      = render_pass_settings.depth_attachment_format,
            .aspect_mask = vk::ImageAspectFlagBits::eDepth,
        } ) );
    }

    if ( !render_pass_.is_initialized( ) )
    {
        LTB_CHECK( render_pass_.initialize( std::move( render_pass_settings ), &swapchain_ ) );
    }

    framebuffers_.reserve( swapchain_image_views_.size( ) );
    for ( auto const& image_view : swapchain_image_views_ )
    {
        auto attachments = std::vector{ image_view.get( ) };
        if ( depth_image_view_.is_initialized( ) )
        {
            attachments.push_back( depth_image_view_.get( ) );
        }

        LTB_CHECK( framebuffers_.emplace_back( gpu_.device( ), render_pass_ )
                       .initialize( {
                           .attachments = attachments,
                           .extent      = swapchain_.settings( ).extent,
                           .layers      = 1U,
                       } ) );
    }
    LTB_CHECK_VALID( swapchain_image_views_.size( ) == framebuffers_.size( ) );

    return utils::success( );
}

} // namespace ltb::vlk::objs
