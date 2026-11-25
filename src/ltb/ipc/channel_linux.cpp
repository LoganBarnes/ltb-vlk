// ///////////////////////////////////////////////////////////////////////////////////////
// A Logan Thomas Barnes project
// ///////////////////////////////////////////////////////////////////////////////////////
// The Linux backend of Channel: Unix domain sockets that pass file descriptors.
#include "ltb/ipc/channel.hpp"

// system
#include <unistd.h>

// standard
#include <cstdlib>
#include <filesystem>
#include <vector>

namespace ltb::ipc
{

auto default_channel_address( std::string_view const name ) -> ChannelAddress
{
    auto const file_name = std::string{ name } + ".sock";

    // The standard per-user directory for sockets on Linux.
    if ( auto const* const runtime_dir = std::getenv( "XDG_RUNTIME_DIR" );
         ( nullptr != runtime_dir ) && ( '\0' != runtime_dir[ 0 ] ) )
    {
        return ( std::filesystem::path{ runtime_dir } / file_name ).string( );
    }
    return ( std::filesystem::temp_directory_path( )
             / ( std::string{ name } + "-" + std::to_string( ::getuid( ) ) + ".sock" ) )
        .string( );
}

// ---------------------------------------------------------------------------------------
// Channel
// ---------------------------------------------------------------------------------------

Channel::Channel( UnixSocket socket )
    : socket_( std::move( socket ) )
{
}

auto Channel::connect( ChannelAddress const& address ) -> utils::Result< Channel >
{
    LTB_CHECK( auto socket, UnixSocket::connect( address ) );
    return Channel{ std::move( socket ) };
}

auto Channel::is_valid( ) const -> bool
{
    return socket_.is_valid( );
}

auto Channel::send(
    std::span< std::byte const > const    bytes,
    std::span< NativeHandle const > const handles
) const -> utils::Result< SendStatus >
{
    auto fds = std::vector< int >{ };
    fds.reserve( handles.size( ) );
    for ( auto const& handle : handles )
    {
        LTB_CHECK_VALID( handle.is_valid( ) );
        fds.push_back( handle.get( ) );
    }
    return socket_.send( bytes, fds );
}

auto Channel::receive( std::span< std::byte > const buffer ) const -> utils::Result< ReceiveResult >
{
    return socket_.receive( buffer );
}

// ---------------------------------------------------------------------------------------
// ChannelListener
// ---------------------------------------------------------------------------------------

ChannelListener::ChannelListener( UnixSocket socket )
    : socket_( std::move( socket ) )
{
}

auto ChannelListener::listen( ChannelAddress const& address ) -> utils::Result< ChannelListener >
{
    LTB_CHECK( auto socket, UnixSocket::listen( address ) );
    return ChannelListener{ std::move( socket ) };
}

auto ChannelListener::is_valid( ) const -> bool
{
    return socket_.is_valid( );
}

auto ChannelListener::accept( ) const -> utils::Result< std::optional< Channel > >
{
    LTB_CHECK( auto socket, socket_.accept( ) );
    if ( !socket.has_value( ) )
    {
        return std::nullopt;
    }
    return Channel{ std::move( socket.value( ) ) };
}

} // namespace ltb::ipc
