// ///////////////////////////////////////////////////////////////////////////////////////
// A Logan Thomas Barnes project
// ///////////////////////////////////////////////////////////////////////////////////////
#if defined( __linux__ )

#include "ltb/ipc/unix_socket.hpp"

// external
#include <gtest/gtest.h>

// system
#include <unistd.h>

// standard
#include <array>
#include <cstdint>
#include <filesystem>
#include <string>

namespace ltb::ipc
{
namespace
{

struct TestMessage
{
    std::uint32_t type  = 0U;
    std::uint64_t value = 0U;
};

auto receive_value( UnixSocket const& socket, TestMessage& value ) -> ReceiveResult
{
    auto result = socket.receive( std::as_writable_bytes( std::span{ &value, 1U } ) );
    EXPECT_TRUE( result.has_value( ) );
    return result.has_value( ) ? std::move( result.value( ) ) : ReceiveResult{ };
}

TEST( UnixSocketTests, ReceiveWouldBlockWhenEmpty )
{
    auto sockets = UnixSocket::pair( );
    ASSERT_TRUE( sockets.has_value( ) );

    auto value = TestMessage{ };
    EXPECT_EQ( receive_value( sockets->second, value ).status, ReceiveStatus::WouldBlock );
}

TEST( UnixSocketTests, MessagesKeepTheirBoundaries )
{
    auto sockets = UnixSocket::pair( );
    ASSERT_TRUE( sockets.has_value( ) );
    auto const& [ sender, receiver ] = sockets.value( );

    ASSERT_TRUE( sender.send_value( TestMessage{ .type = 1U, .value = 10U } ).has_value( ) );
    ASSERT_TRUE( sender.send_value( TestMessage{ .type = 2U, .value = 20U } ).has_value( ) );

    auto value  = TestMessage{ };
    auto result = receive_value( receiver, value );
    ASSERT_EQ( result.status, ReceiveStatus::Message );
    EXPECT_EQ( result.message.size, sizeof( TestMessage ) );
    EXPECT_EQ( value.type, 1U );
    EXPECT_EQ( value.value, 10U );

    result = receive_value( receiver, value );
    ASSERT_EQ( result.status, ReceiveStatus::Message );
    EXPECT_EQ( value.type, 2U );
    EXPECT_EQ( value.value, 20U );
}

TEST( UnixSocketTests, PassesFileDescriptors )
{
    auto sockets = UnixSocket::pair( );
    ASSERT_TRUE( sockets.has_value( ) );
    auto const& [ sender, receiver ] = sockets.value( );

    // Send the write end of a pipe, then write through the received copy.
    auto pipe_fds = std::array{ -1, -1 };
    ASSERT_EQ( ::pipe( pipe_fds.data( ) ), 0 );
    auto const read_end  = FileDescriptor{ pipe_fds[ 0 ] };
    auto       write_end = FileDescriptor{ pipe_fds[ 1 ] };

    auto const fds = std::array{ write_end.get( ) };
    ASSERT_TRUE( sender.send_value( TestMessage{ .type = 3U }, fds ).has_value( ) );
    write_end.reset( ); // The receiver gets its own copy.

    auto value  = TestMessage{ };
    auto result = receive_value( receiver, value );
    ASSERT_EQ( result.status, ReceiveStatus::Message );
    ASSERT_EQ( result.message.fds.size( ), 1U );

    auto const text = std::string{ "hi" };
    ASSERT_EQ( ::write( result.message.fds[ 0 ].get( ), text.data( ), text.size( ) ), 2 );

    auto buffer = std::array< char, 2 >{ };
    ASSERT_EQ( ::read( read_end.get( ), buffer.data( ), buffer.size( ) ), 2 );
    EXPECT_EQ( std::string( buffer.data( ), buffer.size( ) ), text );
}

TEST( UnixSocketTests, ReportsClosedConnections )
{
    auto sockets = UnixSocket::pair( );
    ASSERT_TRUE( sockets.has_value( ) );

    sockets->first = UnixSocket{ };

    auto value = TestMessage{ };
    EXPECT_EQ( receive_value( sockets->second, value ).status, ReceiveStatus::Closed );
}

TEST( UnixSocketTests, SendingToAClosedConnectionReportsClosed )
{
    auto sockets = UnixSocket::pair( );
    ASSERT_TRUE( sockets.has_value( ) );

    sockets->second = UnixSocket{ };

    auto const sent = sockets->first.send_value( TestMessage{ } );
    ASSERT_TRUE( sent.has_value( ) ) << sent.error( ).error_message( );
    EXPECT_EQ( sent.value( ), SendStatus::Closed );
}

TEST( UnixSocketTests, RejectsMessagesLargerThanTheBuffer )
{
    auto sockets = UnixSocket::pair( );
    ASSERT_TRUE( sockets.has_value( ) );

    ASSERT_TRUE( sockets->first.send_value( TestMessage{ } ).has_value( ) );

    auto small = std::uint32_t{ 0U };
    EXPECT_FALSE(
        sockets->second.receive( std::as_writable_bytes( std::span{ &small, 1U } ) ).has_value( )
    );
}

TEST( UnixSocketTests, ListenAcceptConnectAndCleanUp )
{
    auto const path = std::filesystem::temp_directory_path( )
                    / ( "ltb_vlk_socket_test_" + std::to_string( ::getpid( ) ) + ".sock" );
    {
        auto listener = UnixSocket::listen( path );
        ASSERT_TRUE( listener.has_value( ) ) << listener.error( ).error_message( );
        EXPECT_TRUE( std::filesystem::exists( path ) );

        auto no_client = listener->accept( );
        ASSERT_TRUE( no_client.has_value( ) );
        EXPECT_FALSE( no_client->has_value( ) );

        auto client = UnixSocket::connect( path );
        ASSERT_TRUE( client.has_value( ) ) << client.error( ).error_message( );

        auto server = listener->accept( );
        ASSERT_TRUE( server.has_value( ) );
        ASSERT_TRUE( server->has_value( ) );

        ASSERT_TRUE( client->send_value( TestMessage{ .type = 4U } ).has_value( ) );
        auto value = TestMessage{ };
        EXPECT_EQ( receive_value( server->value( ), value ).status, ReceiveStatus::Message );
        EXPECT_EQ( value.type, 4U );
    }
    EXPECT_FALSE( std::filesystem::exists( path ) );
}

TEST( UnixSocketTests, ConnectWithoutListenerFails )
{
    auto const path = std::filesystem::temp_directory_path( ) / "ltb_vlk_socket_test_missing.sock";
    EXPECT_FALSE( UnixSocket::connect( path ).has_value( ) );
}

} // namespace
} // namespace ltb::ipc

#endif // defined( __linux__ )
