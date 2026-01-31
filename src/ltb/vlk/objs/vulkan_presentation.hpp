// ///////////////////////////////////////////////////////////////////////////////////////
// A Logan Thomas Barnes project
// ///////////////////////////////////////////////////////////////////////////////////////
#pragma once

// project
#include "ltb/vlk/device_memory.hpp"
#include "ltb/vlk/framebuffer.hpp"
#include "ltb/vlk/image.hpp"
#include "ltb/vlk/image_view.hpp"
#include "ltb/vlk/objs/vulkan_gpu.hpp"
#include "ltb/vlk/objs/vulkan_image.hpp"
#include "ltb/vlk/render_pass.hpp"
#include "ltb/vlk/swapchain.hpp"

namespace ltb::vlk::objs
{

enum class ExtentMode
{
    FromSurface,
    Custom,
};

struct VulkanPresentationSettings
{
    ExtentMode         extent_mode = ExtentMode::FromSurface;
    SwapchainSettings  swapchain   = { };
    RenderPassSettings render_pass = { };
};

enum class Rebuild
{
    Always,
    IfSizeChanged,
};

struct VulkanPresentationRebuildSettings
{
    Rebuild           rebuild     = Rebuild::IfSizeChanged;
    ExtentMode        extent_mode = ExtentMode::FromSurface;
    SwapchainSettings swapchain   = { };
};

struct BeginRenderPassSettings
{
    vk::CommandBuffer const& command_buffer;
    uint32                   image_index       = std::numeric_limits< uint32 >::max( );
    glm::vec4                color_clear_value = glm::vec4( 0.0F, 0.0F, 0.0F, 1.0F );
    float32                  depth_clear_value = 1.0F;

    std::optional< vk::Rect2D >   render_area = std::nullopt;
    std::optional< vk::Viewport > viewport    = std::nullopt;
    std::optional< vk::Rect2D >   scissor     = std::nullopt;
};

class VulkanPresentation
{
public:
    explicit( false ) VulkanPresentation( VulkanGpu& gpu );

    auto initialize( VulkanPresentationSettings settings ) -> utils::Result< void >;
    auto rebuild( VulkanPresentationRebuildSettings settings ) -> utils::Result< void >;

    [[nodiscard( "Const getter" )]]
    auto is_initialized( ) const -> bool;

    auto begin_render_pass( BeginRenderPassSettings const& settings ) -> utils::Result< void >;

    [[nodiscard( "Const getter" )]]
    auto swapchain( ) const -> Swapchain const&;
    auto swapchain( ) -> Swapchain&;

    [[nodiscard( "Const getter" )]]
    auto swapchain_image_views( ) const -> std::vector< ImageView > const&;
    auto swapchain_image_views( ) -> std::vector< ImageView >&;

    [[nodiscard( "Const getter" )]]
    auto depth_image( ) const -> Image const&;
    auto depth_image( ) -> Image&;

    [[nodiscard( "Const getter" )]]
    auto depth_memory( ) const -> DeviceMemory const&;
    auto depth_memory( ) -> DeviceMemory&;

    [[nodiscard( "Const getter" )]]
    auto depth_image_view( ) const -> ImageView const&;
    auto depth_image_view( ) -> ImageView&;

    [[nodiscard( "Const getter" )]]
    auto render_pass( ) const -> RenderPass const&;
    auto render_pass( ) -> RenderPass&;

    [[nodiscard( "Const getter" )]]
    auto framebuffers( ) const -> std::vector< Framebuffer > const&;
    auto framebuffers( ) -> std::vector< Framebuffer >&;

private:
    VulkanGpu& gpu_;

    Swapchain swapchain_ = {
        gpu_.instance( ),
        gpu_.surface( ),
        gpu_.physical_device( ),
        gpu_.device( ),
    };
    std::vector< ImageView > swapchain_image_views_ = { };

    VulkanImage d_image_          = { gpu_ };
    ImageView   depth_image_view_ = { gpu_.device( ) };

    RenderPass                 render_pass_  = { gpu_.device( ) };
    std::vector< Framebuffer > framebuffers_ = { };

    bool initialized_ = false;

    auto initialize_swapchain(
        SwapchainSettings  swapchain_settings,
        RenderPassSettings render_pass_settings
    ) -> utils::Result< void >;
};

} // namespace ltb::vlk::objs
