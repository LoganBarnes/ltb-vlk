// ///////////////////////////////////////////////////////////////////////////////////////
// A Logan Thomas Barnes project
// ///////////////////////////////////////////////////////////////////////////////////////
#include "ltb/window/glfw_context.hpp"

// external
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <spdlog/spdlog.h>

namespace ltb::window
{
namespace
{
auto default_glfw_error_callback( int32_t const error, char const* const description ) -> void
{
    spdlog::error( "GLFW Error ({}): {}", error, description );
}

} // namespace

auto GlfwContext::initialize( ) -> utils::Result< void >
{
    if ( is_initialized( ) )
    {
        return utils::success( );
    }

    // Set the error callback before any GLFW calls to log when things go wrong.
    // The previous callback is ignored because it doesn't need to be restored.
    utils::ignore( ::glfwSetErrorCallback( default_glfw_error_callback ) );

    // Initialize the window framework library.
    auto glfw = std::make_unique< ScopedGlfw >( );
    if ( GLFW_FALSE == glfw->init_value )
    {
        return LTB_MAKE_UNEXPECTED_ERROR( "::glfwInit() failed" );
    }
    spdlog::debug( "::glfwInit()" );

    // No more errors can occur, so the resources can be moved.
    glfw_ = std::move( glfw );

    return utils::success( );
}

auto GlfwContext::is_initialized( ) const -> bool
{
    return nullptr != glfw_;
}

auto GlfwContext::poll_events( ) const -> void
{
    if ( !is_initialized( ) )
    {
        spdlog::warn( "GlfwContext::poll_events(): not initialized" );
        return;
    }

    ::glfwPollEvents( );
}

auto GlfwContext::wait_for_events( ) const -> void
{
    if ( !is_initialized( ) )
    {
        spdlog::warn( "GlfwContext::wait_for_events(): not initialized" );
        return;
    }

    ::glfwWaitEvents( );
}

auto GlfwContext::get_vulkan_instance_extensions( ) const -> std::vector< char const* >
{
    if ( !is_initialized( ) )
    {
        spdlog::warn( "GlfwContext::get_vulkan_instance_extensions(): not initialized" );
        return { };
    }

    auto               count      = uint32_t{ 0 };
    auto const** const extensions = ::glfwGetRequiredInstanceExtensions( &count );
    return { extensions, extensions + count };
}

} // namespace ltb::window
