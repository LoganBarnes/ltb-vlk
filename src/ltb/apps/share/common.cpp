// ///////////////////////////////////////////////////////////////////////////////////////
// A Logan Thomas Barnes project
// ///////////////////////////////////////////////////////////////////////////////////////
#include "common.hpp"

// project
#include "ltb/utils/ignore.hpp"

// system
#if defined( _WIN32 )
#include <process.h>
#else
#include <unistd.h>
#endif

// standard
#include <algorithm>
#include <csignal>
#include <cstdlib>
#include <cstring>
#include <string>

namespace ltb::share
{
namespace
{

std::sig_atomic_t volatile stop_signal_received = 0;

extern "C" auto handle_stop_signal( int const signal ) -> void
{
    utils::ignore( signal );
    stop_signal_received = 1;
}

} // namespace

auto shared_image_settings( vk::Extent2D const extent ) -> vlk::SharedImageSettings
{
    return {
        .extent = extent,
        .format = shared_image_format,
        // The producer renders into it, and the display copies out of it.
        .usage = vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransferSrc,
    };
}

auto make_rejected_message( std::string_view const reason ) -> RejectedMessage
{
    auto       message = RejectedMessage{ };
    auto const length  = std::min( reason.size( ), message.reason.size( ) - 1U );
    std::copy_n( reason.begin( ), length, message.reason.begin( ) );
    return message;
}

auto rejection_reason( RejectedMessage const& message ) -> std::string
{
    auto const end = std::ranges::find( message.reason, '\0' );
    return std::string( message.reason.begin( ), end );
}

auto peek_message_type( std::span< std::byte const > const bytes ) -> std::optional< MessageType >
{
    if ( bytes.size( ) < sizeof( MessageType ) )
    {
        return std::nullopt;
    }
    auto type = MessageType{ };
    std::memcpy( &type, bytes.data( ), sizeof( type ) );
    return type;
}

auto current_process_id( ) -> int32
{
#if defined( _WIN32 )
    return static_cast< int32 >( ::_getpid( ) );
#else
    return static_cast< int32 >( ::getpid( ) );
#endif
}

auto install_stop_signal_handlers( ) -> void
{
    std::signal( SIGINT, handle_stop_signal );
    std::signal( SIGTERM, handle_stop_signal );
}

auto stop_requested( ) -> bool
{
    return 0 != stop_signal_received;
}

} // namespace ltb::share
