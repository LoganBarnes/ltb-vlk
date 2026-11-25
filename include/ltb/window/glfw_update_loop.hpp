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

/// \brief Runs `app` until it requests an exit or the window closes. The initial loop
///        status comes from `app.initialize()`. `app.clean_up()` always runs once the app
///        has initialized, even if the loop fails, so GPU work can finish before teardown.
template < typename App >
    requires exec::IsUpdatable< App > && exec::HasCleanUp< App >
auto run_update_loop( GlfwContext& glfw, GlfwWindow& window, App& app ) -> utils::Result< void >
{
    LTB_CHECK( glfw.initialize( ) );
    LTB_CHECK( window.initialize( ) );

    auto status = exec::UpdateLoopStatus{ };
    LTB_CHECK( status, app.initialize( ) );

    return exec::run_then_clean_up( app, [ & ]( ) -> utils::Result< void > {
        auto internal_state = exec::InternalLoopState{ status.update_time_step };

        while ( ( !window.should_close( ) ) && ( !status.requests.exit_update_loop ) )
        {
            glfw.poll_events( );

            // A minimized window has a 0x0 framebuffer, which Vulkan can't create a
            // swapchain for. Sleep until something happens instead of updating or rendering.
            if ( auto const size = window.framebuffer_size( ); ( size.x <= 0 ) || ( size.y <= 0 ) )
            {
                window.reset_callback_data( );
                glfw.wait_for_events( );
                continue;
            }

            if ( auto const new_size = window.resized( ) )
            {
                LTB_CHECK( app.on_resize( new_size.value( ) ) );
            }

            single_loop_iteration( internal_state, status, app );

            window.reset_callback_data( );
        }

        return utils::success( );
    } );
}

} // namespace ltb::window
