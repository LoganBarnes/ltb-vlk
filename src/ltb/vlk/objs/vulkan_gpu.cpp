// ///////////////////////////////////////////////////////////////////////////////////////
// A Logan Thomas Barnes project
// ///////////////////////////////////////////////////////////////////////////////////////
#include "ltb/vlk/objs/vulkan_gpu.hpp"

// project
#include "ltb/vlk/check.hpp"
#include "ltb/vlk/graphics_pipeline.hpp"
#include "ltb/window/glfw_context.hpp"
#include "ltb/window/glfw_window.hpp"

// external
#include <spdlog/spdlog.h>

namespace ltb::vlk::objs
{

VulkanGpu::VulkanGpu( ) = default;

VulkanGpu::VulkanGpu( window::GlfwContext& glfw_context, window::GlfwWindow& glfw_window )
    : glfw_context_( &glfw_context )
    , glfw_window_( &glfw_window )
{
    surface_.emplace( glfw_window, instance_ );
}

auto VulkanGpu::initialize( VulkanGpuSettings settings ) -> utils::Result< void >
{
    if ( this->is_initialized( ) )
    {
        return utils::success( );
    }

    if ( this->has_surface( ) )
    {
        LTB_CHECK_VALID( glfw_context_->is_initialized( ) );
        LTB_CHECK_VALID( glfw_window_->is_initialized( ) );

        LTB_CHECK( instance_.initialize( std::move( settings.instance ), glfw_context_ ) );
        LTB_CHECK( surface_->initialize( ) );
        LTB_CHECK(
            physical_device_.initialize( std::move( settings.device ), &surface_.value( ) )
        );
    }
    else
    {
        // Presentation extensions require a surface so they can't be enabled without one.
        auto const is_presentation_extension = []( char const* const extension ) {
            auto const name = std::string_view{ extension };
            return ( name == VK_KHR_SWAPCHAIN_EXTENSION_NAME )
                || ( name == VK_KHR_PRESENT_MODE_FIFO_LATEST_READY_EXTENSION_NAME );
        };
        std::erase_if( settings.device.extensions, is_presentation_extension );
        std::erase_if( settings.device.optional_extensions, is_presentation_extension );

        LTB_CHECK( instance_.initialize( std::move( settings.instance ), nullptr ) );
        LTB_CHECK( physical_device_.initialize( std::move( settings.device ), nullptr ) );
    }
    LTB_CHECK( device_.initialize( ) );
    LTB_CHECK( descriptor_pool_.initialize( std::move( settings.descriptor_pool ) ) );

    initialized_ = true;

    return utils::success( );
}

auto VulkanGpu::is_initialized( ) const -> bool
{
    return initialized_;
}

auto VulkanGpu::instance( ) const -> Instance const&
{
    return instance_;
}

auto VulkanGpu::instance( ) -> Instance&
{
    return instance_;
}

auto VulkanGpu::has_surface( ) const -> bool
{
    return surface_.has_value( );
}

auto VulkanGpu::surface( ) const -> Surface const&
{
    return surface_.value( );
}

auto VulkanGpu::surface( ) -> Surface&
{
    return surface_.value( );
}

auto VulkanGpu::physical_device( ) const -> PhysicalDevice const&
{
    return physical_device_;
}

auto VulkanGpu::physical_device( ) -> PhysicalDevice&
{
    return physical_device_;
}

auto VulkanGpu::device( ) const -> Device const&
{
    return device_;
}

auto VulkanGpu::device( ) -> Device&
{
    return device_;
}

auto VulkanGpu::descriptor_pool( ) const -> DescriptorPool const&
{
    return descriptor_pool_;
}

auto VulkanGpu::descriptor_pool( ) -> DescriptorPool&
{
    return descriptor_pool_;
}

} // namespace ltb::vlk::objs
