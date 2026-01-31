// ///////////////////////////////////////////////////////////////////////////////////////
// A Logan Thomas Barnes project
// ///////////////////////////////////////////////////////////////////////////////////////
#include "ltb/vlk/surface.hpp"

// project
#include "ltb/vlk/check.hpp"
#include "ltb/vlk/instance.hpp"

// external
#include <spdlog/spdlog.h>

namespace ltb::vlk
{

Surface::Surface( window::GlfwWindow& glfw_window, Instance& instance )
    : glfw_window_( glfw_window )
    , instance_( instance )
{
}

auto Surface::initialize( ) -> utils::Result< void >
{
    if ( this->is_initialized( ) )
    {
        return utils::success( );
    }
    LTB_CHECK_VALID( glfw_window_.is_initialized( ) );
    LTB_CHECK_VALID( instance_.is_initialized( ) );

    auto* surface = VkSurfaceKHR{ };
    VK_CHECK(
        vk::Result{
            ::glfwCreateWindowSurface( instance_.get( ), glfw_window_.get( ), nullptr, &surface )
        }
    );

    surface_ = vk::UniqueSurfaceKHR{ surface, instance_.get( ) };
    spdlog::debug( "::glfwCreateWindowSurface()" );

    return utils::success( );
}

auto Surface::is_initialized( ) const -> bool
{
    return nullptr != surface_.get( );
}

auto Surface::get( ) const -> vk::SurfaceKHR const&
{
    return surface_.get( );
}

auto Surface::get( ) -> vk::SurfaceKHR&
{
    return surface_.get( );
}

auto Surface::framebuffer_size( ) const -> vk::Extent2D
{
    auto width  = 0;
    auto height = 0;
    ::glfwGetFramebufferSize( glfw_window_.get( ), &width, &height );

    return {
        static_cast< uint32 >( width ),
        static_cast< uint32 >( height ),
    };
}

} // namespace ltb::vlk
