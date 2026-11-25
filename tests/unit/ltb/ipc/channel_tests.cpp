// ///////////////////////////////////////////////////////////////////////////////////////
// A Logan Thomas Barnes project
// ///////////////////////////////////////////////////////////////////////////////////////
#if defined( __linux__ )

#include "ltb/ipc/channel.hpp"

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

struct Ping
{
    std::uint32_t value = 0U;
};

class ChannelTest : public ::testing::Test
{
protected:
    void SetUp( ) override
    {
        auto listener = ChannelListener::listen( address_ );
        ASSERT_TRUE( listener.has_value( ) ) << listener.error( ).error_message( );
        listener_ = std::move( listener.value( ) );

        auto client = Channel::connect( address_ );
        ASSERT_TRUE( client.has_value( ) ) << client.error( ).error_message( );
        client_ = std::move( client.value( ) );

        auto server = listener_.accept( );
        ASSERT_TRUE( server.has_value( ) );
        ASSERT_TRUE( server->has_value( ) );
        server_ = std::move( server->value( ) );
    }

    ChannelAddress const address_
        = default_channel_address( "ltb-vlk-channel-test-" + std::to_string( ::getpid( ) ) );

    ChannelListener listener_ = { };
    Channel         client_   = { };
    Channel         server_   = { };
};

TEST( ChannelAddressTests, DefaultAddressIsPerNameAndUser )
{
    auto const address = default_channel_address( "some-app" );
    EXPECT_NE( address.find( "some-app" ), std::string::npos );
    EXPECT_NE( address, default_channel_address( "other-app" ) );
}

TEST_F( ChannelTest, SendsMessagesAndHandles )
{
    auto pipe_fds = std::array{ -1, -1 };
    ASSERT_EQ( ::pipe( pipe_fds.data( ) ), 0 );
    auto const read_end   = NativeHandle{ pipe_fds[ 0 ] };
    auto const write_ends = std::array{ NativeHandle{ pipe_fds[ 1 ] } };

    auto const sent = client_.send_value( Ping{ .value = 7U }, write_ends );
    ASSERT_TRUE( sent.has_value( ) ) << sent.error( ).error_message( );
    EXPECT_EQ( sent.value( ), SendStatus::Sent );

    auto ping     = Ping{ };
    auto received = server_.receive( std::as_writable_bytes( std::span{ &ping, 1U } ) );
    ASSERT_TRUE( received.has_value( ) );
    ASSERT_EQ( received->status, ReceiveStatus::Message );
    EXPECT_EQ( ping.value, 7U );
    ASSERT_EQ( received->message.handles.size( ), 1U );

    // The received handle is a working copy of the write end.
    auto const byte = char{ 'x' };
    ASSERT_EQ( ::write( received->message.handles[ 0 ].get( ), &byte, 1U ), 1 );
    auto read_byte = char{ };
    ASSERT_EQ( ::read( read_end.get( ), &read_byte, 1U ), 1 );
    EXPECT_EQ( read_byte, 'x' );
}

TEST_F( ChannelTest, RejectsInvalidHandles )
{
    auto const invalid = std::array{ NativeHandle{} };
    EXPECT_FALSE( client_.send_value( Ping{ }, invalid ).has_value( ) );
}

TEST_F( ChannelTest, ReportsAClosedPeer )
{
    client_ = Channel{ };

    auto ping     = Ping{ };
    auto received = server_.receive( std::as_writable_bytes( std::span{ &ping, 1U } ) );
    ASSERT_TRUE( received.has_value( ) );
    EXPECT_EQ( received->status, ReceiveStatus::Closed );

    auto const sent = server_.send_value( Ping{ } );
    ASSERT_TRUE( sent.has_value( ) );
    EXPECT_EQ( sent.value( ), SendStatus::Closed );
}

} // namespace
} // namespace ltb::ipc

#endif // defined( __linux__ )
