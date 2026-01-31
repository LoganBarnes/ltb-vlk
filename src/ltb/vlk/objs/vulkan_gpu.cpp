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

VulkanGpu::VulkanGpu( window::GlfwContext& glfw_context, window::GlfwWindow& glfw_window )
    : glfw_context_( glfw_context )
    , glfw_window_( glfw_window )
{
}

auto VulkanGpu::initialize( VulkanGpuSettings settings ) -> utils::Result< void >
{
    if ( this->is_initialized( ) )
    {
        return utils::success( );
    }
    LTB_CHECK_VALID( glfw_context_.is_initialized( ) );
    LTB_CHECK_VALID( glfw_window_.is_initialized( ) );

    LTB_CHECK( instance_.initialize( std::move( settings.instance ), &glfw_context_ ) );
    LTB_CHECK( surface_.initialize( ) );
    LTB_CHECK( physical_device_.initialize( std::move( settings.device ), &surface_ ) );
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

auto VulkanGpu::surface( ) const -> Surface const&
{
    return surface_;
}

auto VulkanGpu::surface( ) -> Surface&
{
    return surface_;
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
