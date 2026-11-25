// ///////////////////////////////////////////////////////////////////////////////////////
// A Logan Thomas Barnes project
// ///////////////////////////////////////////////////////////////////////////////////////
#pragma once

// program
#include "ltb/utils/duration.hpp"
#include "ltb/utils/result.hpp"
#include "ltb/utils/types.hpp"

// external
#include <glm/glm.hpp>

// standard
#include <concepts>
#include <utility>

namespace ltb::exec
{

struct UpdateRequests
{
    bool            pause_updates    = false;
    bool            exit_update_loop = false;
    utils::Duration update_time_step = utils::duration_seconds( 1.0 / 60.0 );
};

struct UpdateLoopStatus
{
    float64         time_scale       = 1.0;
    utils::Duration cumulative_time  = utils::Duration::zero( );
    utils::Duration update_time_step = utils::duration_seconds( 1.0 / 60.0 );
    utils::Duration max_frame_time   = utils::duration_seconds( 0.1 );

    // 0.0 is the previous update, 1.0 is the next update.
    float64 interpolant_between_updates = 0.0;

    UpdateRequests requests = { };
};

struct InternalLoopState
{
    explicit InternalLoopState( utils::Duration update_time_step )
        : previous_time( std::chrono::steady_clock::now( ) )
        , accumulator( update_time_step )
    {
    }

    std::chrono::steady_clock::time_point previous_time;

    // The accumulator maintains the relationship between cpu time and
    // simulation time. It is manipulated in two ways:
    //
    // - On each frame the accumulator increases by the time it took
    //   to run the previous frame (cpu time).
    // - After each simulation update the accumulator is decremented
    //   by the constant simulation time step (sim time).
    //
    // This loop will continuously update the simulation until the
    // simulation's time has caught up to the expected cpu "real time".
    // This is done by comparing the accumulator's value to the expected
    // simulation time step. If the accumulator is less than the time step
    // interval, the loop will be allowed to continue on to rendering.
    utils::Duration accumulator;
};

template < typename Object >
concept IsUpdatable = requires( Object obj, UpdateLoopStatus const& status, glm::ivec2 size ) {
    { obj.initialize( ) } -> std::same_as< utils::Result< UpdateLoopStatus > >;
    { obj.fixed_step_update( status ) } -> std::same_as< UpdateRequests >;
    { obj.frame_update( status ) } -> std::same_as< UpdateRequests >;
    { obj.on_resize( size ) } -> std::same_as< utils::Result< void > >;
};

/// \brief Apps that need to release resources (e.g. wait for the GPU) when the loop ends.
template < typename Object >
concept HasCleanUp = requires( Object obj ) {
    { obj.clean_up( ) } -> std::same_as< utils::Result< void > >;
};

/// \brief Runs `loop` and then always calls `app.clean_up()` (if the app has one),
///        even when `loop` returns an error. The loop's error takes precedence.
template < typename App, typename Loop >
auto run_then_clean_up( App& app, Loop&& loop ) -> utils::Result< void >
{
    auto loop_result = std::forward< Loop >( loop )( );

    if constexpr ( HasCleanUp< App > )
    {
        auto clean_up_result = app.clean_up( );
        if ( loop_result && !clean_up_result )
        {
            return clean_up_result;
        }
    }

    return loop_result;
}

// Stores the app's requests and adopts the requested time step (ignored if not positive so
// the fixed update loop always terminates).
inline auto apply_requests( UpdateLoopStatus& status, UpdateRequests const& requests ) -> void
{
    status.requests = requests;

    if ( requests.update_time_step > utils::Duration::zero( ) )
    {
        status.update_time_step = requests.update_time_step;
    }
}

template < typename App >
    requires IsUpdatable< App >
auto single_loop_iteration( InternalLoopState& internal_state, UpdateLoopStatus& status, App& app )
    -> void
{
    auto const new_time          = std::chrono::steady_clock::now( );
    auto       frame_time        = new_time - internal_state.previous_time;
    internal_state.previous_time = new_time;

    frame_time = std::min( status.max_frame_time, frame_time );
    frame_time = utils::duration_millis( utils::to_millis( frame_time ) * status.time_scale );

    // Ignore updates when paused.
    if ( !status.requests.pause_updates )
    {
        internal_state.accumulator += frame_time;

        // Continually update the simulation until the sim
        // time is within one time step of the cpu time.
        while ( internal_state.accumulator >= status.update_time_step )
        {
            status.cumulative_time += status.update_time_step;
            internal_state.accumulator -= status.update_time_step;

            apply_requests( status, app.fixed_step_update( status ) );
        }

        // `interpolant_between_updates` is the normalized (0,1] interpolation value
        // between the last update and the current update when this render call is made
        // (optionally used to interpolate between previous and current states).
        status.interpolant_between_updates
            = utils::to_seconds< float64 >( internal_state.accumulator )
            / utils::to_seconds< float64 >( status.update_time_step );
    }

    apply_requests( status, app.frame_update( status ) );
}

/// \brief Runs `app` until it requests an exit. The initial loop status comes from
///        `app.initialize()`. `app.clean_up()` (if present) runs once the loop ends.
template < typename App >
    requires IsUpdatable< App >
auto run_update_loop( App& app ) -> utils::Result< void >
{
    auto status = UpdateLoopStatus{ };
    LTB_CHECK( status, app.initialize( ) );

    return run_then_clean_up( app, [ & ]( ) -> utils::Result< void > {
        auto internal_state = InternalLoopState{ status.update_time_step };

        while ( !status.requests.exit_update_loop )
        {
            single_loop_iteration( internal_state, status, app );
        }

        return utils::success( );
    } );
}

} // namespace ltb::exec
