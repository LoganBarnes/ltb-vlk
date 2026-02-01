// ///////////////////////////////////////////////////////////////////////////////////////
// A Logan Thomas Barnes project
// ///////////////////////////////////////////////////////////////////////////////////////
#pragma once

// project
#include "ltb/window/glfw_context.hpp"
#include "ltb/window/glfw_update_loop.hpp"
#include "ltb/window/glfw_window.hpp"

// external
#include <spdlog/spdlog.h>

namespace ltb::exec
{
namespace detail
{

template < typename WindowedApp >
auto windowed_app_main_impl( window::WindowSettings window_settings ) -> utils::Result< void >
{
#if !defined( NDEBUG )
    spdlog::set_level( spdlog::level::debug );
#endif

    auto glfw   = window::GlfwContext{ };
    auto window = window::GlfwWindow{ glfw, std::move( window_settings ) };

    auto app = WindowedApp{ glfw, window };

    return window::run_update_loop( glfw, window, app );
}

} // namespace detail

template < typename WindowedApp >
auto windowed_app_main( window::WindowSettings window_settings ) -> int32
{
    if ( auto result
         = detail::windowed_app_main_impl< WindowedApp >( std::move( window_settings ) ) )
    {
        spdlog::info( "Exiting without errors" );
        return EXIT_SUCCESS;
    }
    else
    {
        spdlog::error( result.error( ).debug_error_message( ) );
        return EXIT_FAILURE;
    }
}

template < typename WindowedApp >
auto windowed_app_main( int32 const argc, char const* const* const argv ) -> int32
{
    auto title = std::string{ "Windowed Application" };
    if ( argc > 0 )
    {
        auto const executable_path = std::filesystem::path{ argv[ 0 ] };

        title = executable_path.filename( ).string( );
    }

    return windowed_app_main< WindowedApp >( window::WindowSettings{ .title = title } );
}

} // namespace ltb::exec
