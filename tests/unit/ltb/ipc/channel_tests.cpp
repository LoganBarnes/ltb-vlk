// ///////////////////////////////////////////////////////////////////////////////////////
// A Logan Thomas Barnes project
// ///////////////////////////////////////////////////////////////////////////////////////
#include "ltb/ipc/channel.hpp"

#if defined( LTB_IPC_SUPPORTED )

// external
#include <gtest/gtest.h>

// system
#if defined( _WIN32 )
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#elif defined( __APPLE__ )
#include <mach/mach.h>
#else
#include <unistd.h>
#endif

// standard
#include <array>
#include <chrono>
#include <cstdint>
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

    // Unique per run, so tests running at the same time don't collide.
    ChannelAddress const address_ = default_channel_address(
        "ltb-vlk-channel-test-"
        + std::to_string( std::chrono::steady_clock::now( ).time_since_epoch( ).count( ) )
    );

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

/// \brief A handle to send, and a way to check a received copy refers to the same object.
///        Linux: the write end of a pipe. Windows: a manual-reset event. macOS: a send right
///        to a port this process receives from.
struct TestObject
{
    NativeHandle handle;
#if defined( __APPLE__ )
    MachReceiveRight receive_right;
#elif !defined( _WIN32 )
    NativeHandle read_end;
#endif

    static auto make( ) -> TestObject
    {
#if defined( _WIN32 )
        return { .handle = NativeHandle{ ::CreateEventW( nullptr, TRUE, FALSE, nullptr ) } };
#elif defined( __APPLE__ )
        auto receive_right = MachReceiveRight::allocate( );
        EXPECT_TRUE( receive_right.has_value( ) );
        auto send_right = receive_right->make_send_right( );
        EXPECT_TRUE( send_right.has_value( ) );
        return {
            .handle        = std::move( send_right.value( ) ),
            .receive_right = std::move( receive_right.value( ) ),
        };
#else
        auto fds = std::array{ -1, -1 };
        EXPECT_EQ( ::pipe( fds.data( ) ), 0 );
        return { .handle = NativeHandle{ fds[ 1 ] }, .read_end = NativeHandle{ fds[ 0 ] } };
#endif
    }

    /// \brief Uses `copy` and checks the effect is visible through the original object.
    auto is_same_object( NativeHandle const& copy ) const -> bool
    {
#if defined( _WIN32 )
        return ::SetEvent( copy.get( ) )
            && ( WAIT_OBJECT_0 == ::WaitForSingleObject( handle.get( ), 0U ) );
#elif defined( __APPLE__ )
        struct Received
        {
            mach_msg_header_t      header;
            mach_msg_max_trailer_t trailer;
        };
        auto sent = mach_msg_header_t{ };
        sent.msgh_bits        = MACH_MSGH_BITS_SET( MACH_MSG_TYPE_COPY_SEND, 0U, 0U, 0U );
        sent.msgh_size        = sizeof( sent );
        sent.msgh_remote_port = copy.get( );
        sent.msgh_id          = 1234;
        auto received         = Received{ };
        return ( MACH_MSG_SUCCESS
                 == ::mach_msg( &sent, MACH_SEND_MSG, sizeof( sent ), 0U, MACH_PORT_NULL, 0U, MACH_PORT_NULL ) )
            && ( MACH_MSG_SUCCESS
                 == ::mach_msg(
                     &received.header,
                     MACH_RCV_MSG | MACH_RCV_TIMEOUT,
                     0U,
                     sizeof( received ),
                     receive_right.get( ),
                     0U,
                     MACH_PORT_NULL
                 ) )
            && ( 1234 == received.header.msgh_id );
#else
        auto const byte = char{ 'x' };
        auto       read = char{ };
        return ( 1 == ::write( copy.get( ), &byte, 1U ) )
            && ( 1 == ::read( read_end.get( ), &read, 1U ) ) && ( 'x' == read );
#endif
    }
};

TEST_F( ChannelTest, SendsMessagesAndHandles )
{
    auto const object  = TestObject::make( );
    auto const handles = std::span{ &object.handle, 1U };
    ASSERT_TRUE( object.handle.is_valid( ) );

    auto const sent = client_.send_value( Ping{ .value = 7U }, handles );
    ASSERT_TRUE( sent.has_value( ) ) << sent.error( ).error_message( );
    EXPECT_EQ( sent.value( ), SendStatus::Sent );

    auto ping     = Ping{ };
    auto received = server_.receive( std::as_writable_bytes( std::span{ &ping, 1U } ) );
    ASSERT_TRUE( received.has_value( ) ) << received.error( ).error_message( );
    ASSERT_EQ( received->status, ReceiveStatus::Message );
    EXPECT_EQ( ping.value, 7U );
    ASSERT_EQ( received->message.handles.size( ), 1U );

    // The received handle is a working copy of the sent one.
    EXPECT_TRUE( object.is_same_object( received->message.handles[ 0 ] ) );
}

TEST_F( ChannelTest, ReceiveWouldBlockWhenEmpty )
{
    auto ping     = Ping{ };
    auto received = server_.receive( std::as_writable_bytes( std::span{ &ping, 1U } ) );
    ASSERT_TRUE( received.has_value( ) );
    EXPECT_EQ( received->status, ReceiveStatus::WouldBlock );
}

TEST_F( ChannelTest, AcceptWithoutAClientReturnsNothing )
{
    auto accepted = listener_.accept( );
    ASSERT_TRUE( accepted.has_value( ) );
    EXPECT_FALSE( accepted->has_value( ) );
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

#endif // defined( LTB_IPC_SUPPORTED )
