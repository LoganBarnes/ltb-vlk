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
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <spdlog/spdlog.h>

// standard
#include <algorithm>
#include <span>
#include <string_view>

namespace ltb::vlk::objs
{
namespace
{

/// \brief Adds the instance extensions GLFW needs to create a surface, skipping duplicates.
auto append_glfw_instance_extensions( std::vector< char const* >& extensions )
    -> utils::Result< void >
{
    auto               count         = uint32_t{ 0 };
    auto const** const glfw_required = ::glfwGetRequiredInstanceExtensions( &count );
    if ( nullptr == glfw_required )
    {
        return LTB_MAKE_UNEXPECTED_ERROR(
            "::glfwGetRequiredInstanceExtensions() failed: GLFW isn't initialized or Vulkan "
            "surfaces aren't supported"
        );
    }

    for ( auto const* const required : std::span{ glfw_required, count } )
    {
        auto const same_name = [ required ]( char const* const extension ) {
            return std::string_view{ extension } == required;
        };
        if ( std::ranges::none_of( extensions, same_name ) )
        {
            extensions.push_back( required );
        }
    }
    return utils::success( );
}

} // namespace

VulkanGpu::VulkanGpu( ) = default;

VulkanGpu::VulkanGpu( window::GlfwContext& glfw_context, window::GlfwWindow& glfw_window )
    : glfw_context_( &glfw_context )
    , glfw_window_( &glfw_window )
{
    surface_.emplace( glfw_window, instance_ );
}

VulkanGpu::VulkanGpu( GLFWwindow* const window )
    : external_window_( window )
{
    surface_.emplace( window, instance_ );
}

auto VulkanGpu::initialize( VulkanGpuSettings settings ) -> utils::Result< void >
{
    if ( this->is_initialized( ) )
    {
        return utils::success( );
    }

    if ( this->has_surface( ) )
    {
        if ( nullptr != glfw_context_ )
        {
            LTB_CHECK_VALID( glfw_context_->is_initialized( ) );
            LTB_CHECK_VALID( glfw_window_->is_initialized( ) );
        }
        else
        {
            LTB_CHECK_VALID( nullptr != external_window_ );
            LTB_CHECK( append_glfw_instance_extensions( settings.instance.extensions ) );
        }

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
