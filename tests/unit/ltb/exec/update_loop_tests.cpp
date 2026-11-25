// ///////////////////////////////////////////////////////////////////////////////////////
// A Logan Thomas Barnes project
// ///////////////////////////////////////////////////////////////////////////////////////
#include "ltb/exec/update_loop.hpp"

// external
#include <gtest/gtest.h>

namespace ltb::exec
{
namespace
{

/// \brief Runs a single frame, then exits. Records what the loop did to it.
struct RecordingApp
{
    UpdateLoopStatus initial_status = { };

    bool                  clean_up_called = false;
    utils::Result< void > clean_up_result = utils::success( );

    float64 time_scale_seen = 0.0;

    auto initialize( ) -> utils::Result< UpdateLoopStatus >
    {
        return initial_status;
    }

    auto fixed_step_update( UpdateLoopStatus const& status ) -> UpdateRequests
    {
        return status.requests;
    }

    auto frame_update( UpdateLoopStatus const& status ) -> UpdateRequests
    {
        time_scale_seen = status.time_scale;

        auto requests             = status.requests;
        requests.exit_update_loop = true;
        return requests;
    }

    auto on_resize( glm::ivec2 ) -> utils::Result< void >
    {
        return utils::success( );
    }

    auto clean_up( ) -> utils::Result< void >
    {
        clean_up_called = true;
        return clean_up_result;
    }
};

TEST( UpdateLoopTests, UsesStatusFromInitializeAndCleansUp )
{
    auto app                      = RecordingApp{ };
    app.initial_status.time_scale = 0.25;

    auto const result = run_update_loop( app );

    EXPECT_TRUE( result.has_value( ) );
    EXPECT_DOUBLE_EQ( app.time_scale_seen, 0.25 );
    EXPECT_TRUE( app.clean_up_called );
}

TEST( RunThenCleanUpTests, CleansUpWhenLoopFails )
{
    auto app = RecordingApp{ };

    auto const result = run_then_clean_up( app, [] -> utils::Result< void > {
        return LTB_MAKE_UNEXPECTED_ERROR( "loop failed" );
    } );

    ASSERT_FALSE( result.has_value( ) );
    EXPECT_EQ( result.error( ).error_message( ), "loop failed" );
    EXPECT_TRUE( app.clean_up_called );
}

TEST( RunThenCleanUpTests, LoopErrorTakesPrecedenceOverCleanUpError )
{
    auto app            = RecordingApp{ };
    app.clean_up_result = LTB_MAKE_UNEXPECTED_ERROR( "clean up failed" );

    auto const result = run_then_clean_up( app, [] -> utils::Result< void > {
        return LTB_MAKE_UNEXPECTED_ERROR( "loop failed" );
    } );

    ASSERT_FALSE( result.has_value( ) );
    EXPECT_EQ( result.error( ).error_message( ), "loop failed" );
}

TEST( RunThenCleanUpTests, ReportsCleanUpErrorWhenLoopSucceeds )
{
    auto app            = RecordingApp{ };
    app.clean_up_result = LTB_MAKE_UNEXPECTED_ERROR( "clean up failed" );

    auto const result
        = run_then_clean_up( app, []( ) -> utils::Result< void > { return utils::success( ); } );

    ASSERT_FALSE( result.has_value( ) );
    EXPECT_EQ( result.error( ).error_message( ), "clean up failed" );
}

} // namespace
} // namespace ltb::exec
