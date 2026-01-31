// ///////////////////////////////////////////////////////////////////////////////////////
// A Logan Thomas Barnes project
// ///////////////////////////////////////////////////////////////////////////////////////
#pragma once

// program
#include "ltb/exec/update_loop.hpp"
#include "ltb/window/glfw_context.hpp"
#include "ltb/window/glfw_window.hpp"

namespace ltb::window
{

template < typename App >
    requires exec::IsUpdatable< App >
auto run_update_loop( GlfwContext& glfw, GlfwWindow& window, App& app ) -> utils::Result< void >
{
    return run_update_loop< App >( glfw, window, app, exec::UpdateLoopStatus{ } );
}

template < typename App >
    requires exec::IsUpdatable< App >
auto run_update_loop(
    GlfwContext&           glfw,
    GlfwWindow&            window,
    App&                   app,
    exec::UpdateLoopStatus status
) -> utils::Result< void >
{
    LTB_CHECK( glfw.initialize( ) );
    LTB_CHECK( window.initialize( ) );
    LTB_CHECK( status, app.initialize( ) );

    auto internal_state = exec::InternalLoopState{ status.update_time_step };

    while ( ( !window.should_close( ) ) && ( !status.requests.exit_update_loop ) )
    {
        glfw.poll_events( );

        if ( auto const new_size = window.resized( ) )
        {
            LTB_CHECK( app.on_resize( new_size.value( ) ) );
        }

        single_loop_iteration( internal_state, status, app );

        window.reset_callback_data( );
    }

    LTB_CHECK( app.clean_up( ) );

    return utils::success( );
}

} // namespace ltb::window
