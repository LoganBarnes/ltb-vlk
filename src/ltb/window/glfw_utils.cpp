// ///////////////////////////////////////////////////////////////////////////////////////
// A Logan Thomas Barnes project
// ///////////////////////////////////////////////////////////////////////////////////////
#include "ltb/window/glfw_utils.hpp"

// external
#include <spdlog/spdlog.h>

namespace ltb::window
{
namespace
{

auto glfw_framebuffer_size_callback(
    GLFWwindow* const window,
    int32_t const     width,
    int32_t const     height
) -> void
{
    auto* const callback_data
        = static_cast< CallbackData* >( ::glfwGetWindowUserPointer( window ) );
    callback_data->resized_framebuffer = glm::ivec2{ width, height };
}

} // namespace

ScopedGlfw::ScopedGlfw( )
    : init_value{ ::glfwInit( ) }
{
}

ScopedGlfw::~ScopedGlfw( )
{
    if ( GLFW_FALSE != init_value )
    {
        ::glfwTerminate( );
        spdlog::debug( "::glfwTerminate( )" );
    }
}

auto GlfwWindowDeleter::operator( )( GLFWwindow* const window ) const -> void
{
    if ( nullptr != window )
    {
        ::glfwDestroyWindow( window );
        spdlog::debug( "::glfwDestroyWindow( )" );
    }
}

auto apply_window_hints( WindowSettings const& settings ) -> utils::Result< void >
{
    ::glfwWindowHint( GLFW_VISIBLE, GLFW_TRUE );

    constexpr auto default_samples_per_pixel = 4;
    ::glfwWindowHint( GLFW_SAMPLES, default_samples_per_pixel );

    if ( settings.transparent_background )
    {
        ::glfwWindowHint( GLFW_TRANSPARENT_FRAMEBUFFER, GLFW_TRUE );
    }
    else
    {
        ::glfwWindowHint( GLFW_TRANSPARENT_FRAMEBUFFER, GLFW_FALSE );
    }

    if ( settings.title_bar )
    {
        ::glfwWindowHint( GLFW_DECORATED, GLFW_TRUE );
    }
    else
    {
        ::glfwWindowHint( GLFW_DECORATED, GLFW_FALSE );
    }

    if ( settings.resizable )
    {
        ::glfwWindowHint( GLFW_RESIZABLE, GLFW_TRUE );
    }
    else
    {
        ::glfwWindowHint( GLFW_RESIZABLE, GLFW_FALSE );
    }

    // Request the Vulkan API.
    ::glfwWindowHint( GLFW_CLIENT_API, GLFW_NO_API );
    if ( GLFW_FALSE == ::glfwVulkanSupported( ) )
    {
        return LTB_MAKE_UNEXPECTED_ERROR( "Vulkan requested but not supported" );
    }

    return utils::success( );
}

auto get_initial_window_size( WindowSettings const& settings ) -> utils::Result< glm::ivec2 >
{
    auto* const monitor = ::glfwGetPrimaryMonitor( );
    if ( nullptr == monitor )
    {
        return LTB_MAKE_UNEXPECTED_ERROR( "Monitor not found" );
    }

    auto const* const video_mode = ::glfwGetVideoMode( monitor );
    if ( nullptr == video_mode )
    {
        return LTB_MAKE_UNEXPECTED_ERROR( "glfwGetVideoMode() failed" );
    }
    ::glfwWindowHint( GLFW_RED_BITS, video_mode->redBits );
    ::glfwWindowHint( GLFW_GREEN_BITS, video_mode->greenBits );
    ::glfwWindowHint( GLFW_BLUE_BITS, video_mode->blueBits );
    ::glfwWindowHint( GLFW_REFRESH_RATE, video_mode->refreshRate );

    auto initial_size = glm::ivec2{ };

    // A specific size was requested.
    if ( settings.initial_size.has_value( ) )
    {
        initial_size = settings.initial_size.value( );

        // Full screen with title bar.
    }
    else if ( settings.title_bar )
    {
        initial_size = glm::ivec2{ video_mode->width, video_mode->height };

        // Full screen without the title bar.
    }
    else
    {
        // Get the work area of the monitor to create a window that fits
        // the screen without overlapping the taskbar or other system UI.
        struct WorkArea
        {
            glm::ivec2 position = { };
            glm::ivec2 size     = { };
        };

        auto work_area = WorkArea{ };

        ::glfwGetMonitorWorkarea(
            monitor,
            &work_area.position.x,
            &work_area.position.y,
            &work_area.size.x,
            &work_area.size.y
        );

        initial_size = work_area.size;
    }

    return initial_size;
}

auto set_callbacks( GLFWwindow* const window ) -> utils::Result< std::unique_ptr< CallbackData > >
{
    auto callback_data = std::make_unique< CallbackData >( );

    ::glfwSetWindowUserPointer( window, callback_data.get( ) );

    // Get the current framebuffer size for any graphics APIs using the window.
    auto framebuffer_size = glm::ivec2{ };
    ::glfwGetFramebufferSize( window, &framebuffer_size.x, &framebuffer_size.y );
    callback_data->resized_framebuffer = framebuffer_size;

    // Ignore the old, returned callback.
    utils::ignore( ::glfwSetFramebufferSizeCallback( window, glfw_framebuffer_size_callback ) );

    return callback_data;
}

} // namespace ltb::window
