// ///////////////////////////////////////////////////////////////////////////////////////
// A Logan Thomas Barnes project
// ///////////////////////////////////////////////////////////////////////////////////////
#include "app.hpp"

// project
#include "ltb/exec/update_loop.hpp"

// external
#include <spdlog/spdlog.h>

namespace ltb
{
namespace
{

auto ltb_main( int32 const argc, char const* argv[] ) -> utils::Result< void >
{
    utils::ignore( argc, argv );

    auto app = HeadlessApp{ };

    return exec::run_update_loop( app );
}

} // namespace
} // namespace ltb

auto main( ltb::int32 const argc, char const* argv[] ) -> int
{
    if ( auto result = ltb::ltb_main( argc, argv ) )
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
