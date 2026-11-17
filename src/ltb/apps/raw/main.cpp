// ///////////////////////////////////////////////////////////////////////////////////////
// A Logan Thomas Barnes project
// ///////////////////////////////////////////////////////////////////////////////////////

// project
#include "app.hpp"
#include "ltb/utils/types.hpp"

// external
#include <spdlog/spdlog.h>

auto main( ltb::int32 const argc, char const* argv[] ) -> int
{
    ltb::utils::ignore( argc, argv );

    auto app = ltb::RawApp{ };
    if ( auto result = app.initialize( ) )
    {
        while ( app.running( ) )
        {
            app.render( );
        }
    }
    else
    {
        spdlog::error(
            "Failed to initialize the application: {}",
            result.error( ).debug_error_message( )
        );
    }

    app.cleanup( );
}
