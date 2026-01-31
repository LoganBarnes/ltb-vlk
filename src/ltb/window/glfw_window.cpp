// ///////////////////////////////////////////////////////////////////////////////////////
// A Logan Thomas Barnes project
// ///////////////////////////////////////////////////////////////////////////////////////
#include "ltb/window/glfw_window.hpp"

// project
#include "ltb/window/glfw_context.hpp"

// external
#include <spdlog/spdlog.h>

namespace ltb::window
{

GlfwWindow::GlfwWindow( GlfwContext& context, WindowSettings settings )
    : context_( context )
    , settings_( std::move( settings ) )
{
}

auto GlfwWindow::initialize( ) -> utils::Result< void >
{
    if ( is_initialized( ) )
    {
        return utils::success( );
    }

    LTB_CHECK( context_.initialize( ) );
    LTB_CHECK( apply_window_hints( settings_ ) );
    LTB_CHECK( auto const initial_size, get_initial_window_size( settings_ ) );

    // Create the window.
    auto window = GlfwWindowHandle{
        ::glfwCreateWindow(
            initial_size.x,
            initial_size.y,
            settings_.title.data( ),
            nullptr,
            nullptr
        ),
    };
    if ( nullptr == window )
    {
        return LTB_MAKE_UNEXPECTED_ERROR( "::glfwCreateWindow() failed" );
    }
    spdlog::debug( "::glfwCreateWindow()" );

    LTB_CHECK( auto callback_data, set_callbacks( window.get( ) ) );

    // No more errors can occur, so the resources can be moved.
    window_        = std::move( window );
    callback_data_ = std::move( callback_data );

    return utils::success( );
}

auto GlfwWindow::is_initialized( ) const -> bool
{
    return window_ && callback_data_;
}

auto GlfwWindow::should_close( ) const -> bool
{
    return ::glfwWindowShouldClose( window_.get( ) );
}

auto GlfwWindow::resized( ) const -> std::optional< glm::ivec2 >
{
    return callback_data_->resized_framebuffer;
}

auto GlfwWindow::reset_callback_data( ) const -> void
{
    callback_data_->resized_framebuffer = std::nullopt;
}

auto GlfwWindow::framebuffer_size( ) const -> glm::ivec2
{
    auto size = glm::ivec2{ 0, 0 };
    ::glfwGetFramebufferSize( window_.get( ), &size.x, &size.y );
    return size;
}

auto GlfwWindow::get( ) const -> GLFWwindow*
{
    return window_.get( );
}

} // namespace ltb::window
