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

struct SwapchainSettings
{
    vk::Extent2D extent = { 0U, 0U };

    std::vector< vk::PresentModeKHR > preferred_present_modes = {
        vk::PresentModeKHR::eFifoLatestReady,
        vk::PresentModeKHR::eFifo,
    };

    std::vector< vk::SurfaceFormatKHR > preferred_surface_formats = {
        { vk::Format::eB8G8R8A8Unorm, vk::ColorSpaceKHR::eSrgbNonlinear },
    };
};

enum class Reinitialize
{
    Always,
    OnlyIfNotInitialized,
};

class Swapchain
{
public:
    Swapchain(
        Instance&       instance,
        Surface&        surface,
        PhysicalDevice& physical_device,
        Device&         device
    );

    auto initialize( SwapchainSettings settings ) -> utils::Result< void >;
    auto initialize( SwapchainSettings settings, Reinitialize reinitialize )
        -> utils::Result< void >;

    [[nodiscard( "Const getter" )]]
    auto is_initialized( ) const -> bool;

    [[nodiscard( "Const getter" )]]
    auto get( ) const -> vk::SwapchainKHR const&;
    auto get( ) -> vk::SwapchainKHR&;

    [[nodiscard( "Const getter" )]]
    auto settings( ) const -> SwapchainSettings const&;

    [[nodiscard( "Const getter" )]]
    auto min_image_count( ) const -> uint32;

    [[nodiscard( "Const getter" )]]
    auto images( ) const -> std::vector< vk::Image > const&;
    auto images( ) -> std::vector< vk::Image >&;

    [[nodiscard( "Const getter" )]]
    auto image_format( ) const -> vk::Format;

private:
    Instance&       instance_;
    Surface&        surface_;
    PhysicalDevice& physical_device_;
    Device&         device_;

    SwapchainSettings        settings_        = { };
    vk::UniqueSwapchainKHR   swapchain_       = { };
    uint32                   min_image_count_ = 0U;
    std::vector< vk::Image > images_          = { };
    vk::Format               image_format_    = { };
};

} // namespace ltb::vlk
