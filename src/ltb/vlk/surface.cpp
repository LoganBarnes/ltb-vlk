// ///////////////////////////////////////////////////////////////////////////////////////
// A Logan Thomas Barnes project
// ///////////////////////////////////////////////////////////////////////////////////////
#include "ltb/vlk/surface.hpp"

// project
#include "ltb/vlk/check.hpp"
#include "ltb/vlk/instance.hpp"
#include "ltb/window/glfw_window.hpp"

// external
#include <spdlog/spdlog.h>

namespace ltb::vlk
{

Surface::Surface( window::GlfwWindow& glfw_window, Instance& instance )
    : glfw_window_( &glfw_window )
    , instance_( instance )
{
}

Surface::Surface( GLFWwindow* const window, Instance& instance )
    : external_window_( window )
    , instance_( instance )
{
}

auto Surface::initialize( ) -> utils::Result< void >
{
    if ( this->is_initialized( ) )
    {
        return utils::success( );
    }
    LTB_CHECK_VALID( nullptr != this->window( ) );
    LTB_CHECK_VALID( instance_.is_initialized( ) );

    auto* surface = VkSurfaceKHR{ };
    VK_CHECK(
        vk::Result{
            ::glfwCreateWindowSurface( instance_.get( ), this->window( ), nullptr, &surface )
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
    ::glfwGetFramebufferSize( this->window( ), &width, &height );

    return {
        static_cast< uint32 >( width ),
        static_cast< uint32 >( height ),
    };
}

auto Surface::window( ) const -> GLFWwindow*
{
    return ( nullptr != glfw_window_ ) ? glfw_window_->get( ) : external_window_;
}

} // namespace ltb::vlk
