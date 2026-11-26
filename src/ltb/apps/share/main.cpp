// ///////////////////////////////////////////////////////////////////////////////////////
// A Logan Thomas Barnes project
// ///////////////////////////////////////////////////////////////////////////////////////
//
// Shares rendered output between processes using Vulkan external memory.
//
//   share-app --display        Opens a window that shows every producer's output.
//   share-app --share          Renders offscreen (no window) and shares the result with the
//                              display. Run as many of these as you like.
//
// The display must be running before producers start. Both must use the same GPU.
//
#include "common.hpp"
#include "display.hpp"
#include "producer.hpp"

// project
#include "ltb/exec/update_loop.hpp"
#include "ltb/window/glfw_context.hpp"
#include "ltb/window/glfw_update_loop.hpp"
#include "ltb/window/glfw_window.hpp"

// external
#include <cxxopts.hpp>
#include <spdlog/spdlog.h>

// standard
#include <cmath>
#include <cstdlib>
#include <iostream>

namespace
{

auto run_display( ltb::ipc::ChannelAddress address ) -> ltb::utils::Result< void >
{
    auto glfw   = ltb::window::GlfwContext{ };
    auto window = ltb::window::GlfwWindow{ glfw, { .title = "Shared Output Display" } };
    auto app    = ltb::share::ShareDisplay{ glfw, window, { .address = std::move( address ) } };

    return ltb::window::run_update_loop( glfw, window, app );
}

auto run_producer( ltb::share::ProducerSettings settings ) -> ltb::utils::Result< void >
{
    auto app = ltb::share::ShareProducer{ std::move( settings ) };
    return ltb::exec::run_update_loop( app );
}

} // namespace

auto main( int const argc, char const* const* const argv ) -> int
{
    auto options = cxxopts::Options( "share-app", "Share rendered output between processes." );

    // clang-format off
    options.add_options( )
        ( "display", "Open a window that shows the output of every producer" )
        ( "share", "Render offscreen and share the output with a running display" )
        ( "socket", "Address the display listens on (a socket path on Linux, a pipe name on Windows, a launchd service name on macOS)",
          cxxopts::value< std::string >( )->default_value(
              ltb::ipc::default_channel_address( ltb::share::default_channel_name ) ) )
        ( "size", "Width and height of a producer's output, in pixels",
          cxxopts::value< ltb::uint32 >( )->default_value( "512" ) )
        ( "hue", "Producer color, 0 to 1 (defaults to one picked from the process id)",
          cxxopts::value< ltb::float32 >( ) )
        ( "fps", "Producer frame rate", cxxopts::value< ltb::float64 >( )->default_value( "60" ) )
        ( "h,help", "Print usage" );
    // clang-format on

    auto parsed = cxxopts::ParseResult{ };
    try
    {
        parsed = options.parse( argc, argv );
    }
    catch ( std::exception const& error )
    {
        std::cerr << error.what( ) << "\n\n" << options.help( ) << std::endl;
        return EXIT_FAILURE;
    }

    auto const display = parsed.count( "display" ) > 0U;
    auto const share   = parsed.count( "share" ) > 0U;
    if ( parsed.count( "help" ) > 0U || ( display == share ) )
    {
        std::cout << options.help( ) << "\nUse exactly one of --display or --share." << std::endl;
        return ( parsed.count( "help" ) > 0U ) ? EXIT_SUCCESS : EXIT_FAILURE;
    }

#if !defined( NDEBUG )
    spdlog::set_level( spdlog::level::debug );
#endif

    // Ctrl+C shuts down cleanly (waits for the GPU, removes the socket file).
    ltb::share::install_stop_signal_handlers( );

    auto const address = ltb::ipc::ChannelAddress{ parsed[ "socket" ].as< std::string >( ) };

    auto result = ltb::utils::Result< void >{ };
    if ( display )
    {
        result = run_display( address );
    }
    else
    {
        auto const size = parsed[ "size" ].as< ltb::uint32 >( );
        auto const fps  = parsed[ "fps" ].as< ltb::float64 >( );
        if ( ( 0U == size ) || ( fps <= 0.0 ) )
        {
            std::cerr << "--size and --fps must be positive" << std::endl;
            return EXIT_FAILURE;
        }

        // Spread hues out so producers started one after another look different.
        constexpr auto golden_ratio_fraction = 0.618034F;
        auto const hue = ( parsed.count( "hue" ) > 0U )
                           ? parsed[ "hue" ].as< ltb::float32 >( )
                           : std::fmod(
                                 static_cast< ltb::float32 >( ltb::share::current_process_id( ) )
                                     * golden_ratio_fraction,
                                 1.0F
                             );

        result = run_producer( {
            .display_address   = address,
            .extent            = { size, size },
            .hue               = hue,
            .frames_per_second = fps,
        } );
    }

    if ( !result )
    {
        spdlog::error( "{}", result.error( ).debug_error_message( ) );
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
