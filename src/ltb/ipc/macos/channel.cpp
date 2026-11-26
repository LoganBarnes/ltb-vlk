// ///////////////////////////////////////////////////////////////////////////////////////
// A Logan Thomas Barnes project
// ///////////////////////////////////////////////////////////////////////////////////////
// The macOS backend of Channel: Mach messages that carry port send rights.
//
// Connecting:
//
//   The listener registers a receive port with launchd under the channel address
//   (bootstrap_check_in). A client looks it up (bootstrap_look_up), allocates two ports,
//   one for each side of the connection, and sends a connect message to the listener
//   with a send right to its own port and the receive right of the other. It keeps a send
//   right to that other port, so it can send messages straight away: they queue up until
//   the listener accepts the connection (receives the connect message), which hands over
//   the port and everything queued on it. Neither side ever waits for the other.
//
// Messages:
//
//   Complex Mach messages: a header, one port descriptor per handle (copied send rights,
//   so the sender keeps its own), then the payload's size and bytes inline.
//
// Disconnecting:
//
//   Each side asks for a no-senders notification on its own port. It arrives (after any
//   messages the peer sent before) once the peer has released its send right, by
//   destroying its Channel or exiting, and is reported as `ReceiveStatus::Closed`.
//   Sending to a peer whose port is gone fails with MACH_SEND_INVALID_DEST, reported as
//   `SendStatus::Closed`.
#include "ltb/ipc/channel.hpp"

// system
#include <mach/mach.h>
#include <servers/bootstrap.h>

// standard
#include <algorithm>
#include <array>
#include <cstdint>
#include <cstring>
#include <limits>
#include <vector>

namespace ltb::ipc
{
namespace
{

/// \brief Message ids ("LTB" followed by a number), so stray messages can be rejected.
constexpr auto connect_message_id = mach_msg_id_t{ 0x4C544201 };
constexpr auto data_message_id    = mach_msg_id_t{ 0x4C544202 };

/// \brief Sent by a client to the listener's port.
struct ConnectMessage
{
    mach_msg_header_t          header;
    mach_msg_body_t            body;
    mach_msg_port_descriptor_t client_port; ///< Send right: where the listener's side sends
    mach_msg_port_descriptor_t listener_port; ///< Receive right: where the client sends
};

struct ReceivedConnectMessage
{
    ConnectMessage         message;
    mach_msg_max_trailer_t trailer;
};

// Byte offsets within a data message.
constexpr auto descriptors_offset = sizeof( mach_msg_header_t ) + sizeof( mach_msg_body_t );

constexpr auto payload_size_offset( std::size_t const handle_count ) -> std::size_t
{
    return descriptors_offset + ( handle_count * sizeof( mach_msg_port_descriptor_t ) );
}

constexpr auto payload_offset( std::size_t const handle_count ) -> std::size_t
{
    return payload_size_offset( handle_count ) + sizeof( mach_msg_size_t );
}

/// \brief Mach message sizes must be multiples of 4 bytes.
constexpr auto round_up_to_4( std::size_t const size ) -> std::size_t
{
    return ( size + 3U ) & ~std::size_t{ 3U };
}

/// \brief Storage for a message, aligned for `mach_msg_header_t`.
class MessageBuffer
{
public:
    explicit MessageBuffer( std::size_t const size )
        : words_( ( size + sizeof( std::uint64_t ) - 1U ) / sizeof( std::uint64_t ), 0U )
    {
    }

    [[nodiscard]] auto header( ) -> mach_msg_header_t*
    {
        return reinterpret_cast< mach_msg_header_t* >( words_.data( ) );
    }

    [[nodiscard]] auto bytes( ) -> std::byte*
    {
        return reinterpret_cast< std::byte* >( words_.data( ) );
    }

    [[nodiscard]] auto capacity( ) const -> std::size_t
    {
        return words_.size( ) * sizeof( std::uint64_t );
    }

    template < typename T >
    auto write( std::size_t const offset, T const& value ) -> void
    {
        std::memcpy( this->bytes( ) + offset, &value, sizeof( T ) );
    }

    template < typename T >
    [[nodiscard]] auto read( std::size_t const offset ) -> T
    {
        auto value = T{ };
        std::memcpy( &value, this->bytes( ) + offset, sizeof( T ) );
        return value;
    }

private:
    std::vector< std::uint64_t > words_;
};

auto mach_error( char const* const function, kern_return_t const result ) -> utils::Error
{
    return LTB_MAKE_ERROR( "{} failed: {} ({})", function, ::mach_error_string( result ), result );
}

/// \brief Asks for a no-senders notification on `port`, sent to `port` itself. If there are
///        no senders already, it's sent straight away.
auto request_no_senders_notification( mach_port_t const port ) -> utils::Result< void >
{
    auto       previous = mach_port_t{ MACH_PORT_NULL };
    auto const result   = ::mach_port_request_notification(
        ::mach_task_self( ),
        port,
        MACH_NOTIFY_NO_SENDERS,
        0U,
        port,
        MACH_MSG_TYPE_MAKE_SEND_ONCE,
        &previous
    );
    if ( KERN_SUCCESS != result )
    {
        return tl::make_unexpected( mach_error( "mach_port_request_notification", result ) );
    }
    // Replaces an earlier request (if any). Dropping its send-once right sends a
    // send-once notification, which `receive` ignores.
    if ( MACH_PORT_VALID( previous ) )
    {
        ::mach_port_deallocate( ::mach_task_self( ), previous );
    }
    return utils::success( );
}

/// \brief Receives one message from `port` into `buffer` without blocking. Messages larger
///        than the buffer are destroyed and reported as MACH_RCV_TOO_LARGE.
auto receive_message( mach_port_t const port, MessageBuffer& buffer ) -> mach_msg_return_t
{
    return ::mach_msg(
        buffer.header( ),
        MACH_RCV_MSG | MACH_RCV_TIMEOUT,
        0U,
        static_cast< mach_msg_size_t >( buffer.capacity( ) ),
        port,
        0U,
        MACH_PORT_NULL
    );
}

/// \brief Whether the descriptor is a port carrying the expected kind of right.
auto is_port_descriptor( mach_msg_port_descriptor_t const& descriptor, unsigned const disposition )
    -> bool
{
    return ( MACH_MSG_PORT_DESCRIPTOR == descriptor.type )
        && ( disposition == descriptor.disposition );
}

} // namespace

auto default_channel_address( std::string_view const name ) -> ChannelAddress
{
    // launchd keeps a separate namespace per user (and login session), so the name alone
    // is enough.
    return "ltb.ipc." + std::string{ name };
}

// ---------------------------------------------------------------------------------------
// Channel
// ---------------------------------------------------------------------------------------

Channel::Channel( MachReceiveRight receive_port, MachSendRight peer_port )
    : receive_port_( std::move( receive_port ) )
    , peer_port_( std::move( peer_port ) )
{
}

auto Channel::connect( ChannelAddress const& address ) -> utils::Result< Channel >
{
    auto listener_port = mach_port_t{ MACH_PORT_NULL };
    if ( auto const result
         = ::bootstrap_look_up( bootstrap_port, address.c_str( ), &listener_port );
         BOOTSTRAP_SUCCESS != result )
    {
        return LTB_MAKE_UNEXPECTED_ERROR(
            "Nothing is listening at '{}' ({})",
            address,
            ::bootstrap_strerror( result )
        );
    }
    auto const listener = MachSendRight{ listener_port };

    // This side's port, and the listener's side of the connection.
    LTB_CHECK( auto receive_port, MachReceiveRight::allocate( ) );
    LTB_CHECK( auto peer_receive_port, MachReceiveRight::allocate( ) );
    LTB_CHECK( auto peer_port, peer_receive_port.make_send_right( ) );

    auto message = ConnectMessage{ };
    message.header.msgh_bits
        = MACH_MSGH_BITS_SET( MACH_MSG_TYPE_COPY_SEND, 0U, 0U, MACH_MSGH_BITS_COMPLEX );
    message.header.msgh_size           = sizeof( message );
    message.header.msgh_remote_port    = listener.get( );
    message.header.msgh_local_port     = MACH_PORT_NULL;
    message.header.msgh_id             = connect_message_id;
    message.body.msgh_descriptor_count = 2U;

    message.client_port.name        = receive_port.get( );
    message.client_port.disposition = MACH_MSG_TYPE_MAKE_SEND;
    message.client_port.type        = MACH_MSG_PORT_DESCRIPTOR;

    message.listener_port.name        = peer_receive_port.get( );
    message.listener_port.disposition = MACH_MSG_TYPE_MOVE_RECEIVE;
    message.listener_port.type        = MACH_MSG_PORT_DESCRIPTOR;

    // A short wait, in case the listener's queue of connections is full.
    constexpr auto connect_timeout_ms = mach_msg_timeout_t{ 1000U };

    auto const result = ::mach_msg(
        &message.header,
        MACH_SEND_MSG | MACH_SEND_TIMEOUT,
        sizeof( message ),
        0U,
        MACH_PORT_NULL,
        connect_timeout_ms,
        MACH_PORT_NULL
    );
    if ( MACH_MSG_SUCCESS != result )
    {
        if ( ( MACH_SEND_TIMED_OUT == result ) || ( MACH_SEND_INTERRUPTED == result ) )
        {
            // The kernel gave the message's rights back. Destroying the message releases
            // them, including the receive right that was being moved.
            ::mach_msg_destroy( &message.header );
            peer_receive_port.release( );
        }
        if ( MACH_SEND_INVALID_DEST == result )
        {
            return LTB_MAKE_UNEXPECTED_ERROR( "Nothing is listening at '{}' any more", address );
        }
        return tl::make_unexpected( mach_error( "mach_msg (connect)", result ) );
    }
    // The listener owns its side of the connection now.
    peer_receive_port.release( );

    // The send right made for the listener counts as a sender while it's in transit.
    LTB_CHECK( request_no_senders_notification( receive_port.get( ) ) );

    return Channel{ std::move( receive_port ), std::move( peer_port ) };
}

auto Channel::is_valid( ) const -> bool
{
    return receive_port_.is_valid( ) && peer_port_.is_valid( );
}

auto Channel::send(
    std::span< std::byte const > const    bytes,
    std::span< NativeHandle const > const handles
) const -> utils::Result< SendStatus >
{
    LTB_CHECK_VALID( this->is_valid( ) );
    LTB_CHECK_VALID( handles.size( ) <= max_handles_per_message, "too many handles" );
    LTB_CHECK_VALID( std::ranges::all_of( handles, &NativeHandle::is_valid ) );

    auto const message_size = round_up_to_4( payload_offset( handles.size( ) ) + bytes.size( ) );
    LTB_CHECK_VALID(
        message_size <= std::numeric_limits< mach_msg_size_t >::max( ),
        "message is too large"
    );

    auto buffer = MessageBuffer{ message_size };

    auto* const header = buffer.header( );
    header->msgh_bits
        = MACH_MSGH_BITS_SET( MACH_MSG_TYPE_COPY_SEND, 0U, 0U, MACH_MSGH_BITS_COMPLEX );
    header->msgh_size        = static_cast< mach_msg_size_t >( message_size );
    header->msgh_remote_port = peer_port_.get( );
    header->msgh_local_port  = MACH_PORT_NULL;
    header->msgh_id          = data_message_id;

    buffer.write(
        sizeof( mach_msg_header_t ),
        mach_msg_body_t{
            .msgh_descriptor_count = static_cast< mach_msg_size_t >( handles.size( ) )
        }
    );
    for ( auto i = 0UZ; i < handles.size( ); ++i )
    {
        auto descriptor        = mach_msg_port_descriptor_t{ };
        descriptor.name        = handles[ i ].get( );
        descriptor.disposition = MACH_MSG_TYPE_COPY_SEND; // The caller keeps its right.
        descriptor.type        = MACH_MSG_PORT_DESCRIPTOR;
        buffer.write( descriptors_offset + ( i * sizeof( descriptor ) ), descriptor );
    }
    buffer.write(
        payload_size_offset( handles.size( ) ),
        static_cast< mach_msg_size_t >( bytes.size( ) )
    );
    if ( !bytes.empty( ) )
    {
        std::memcpy(
            buffer.bytes( ) + payload_offset( handles.size( ) ),
            bytes.data( ),
            bytes.size( )
        );
    }

    // Never wait: a full queue (over a thousand unread messages) is reported as an error.
    auto const result = ::mach_msg(
        header,
        MACH_SEND_MSG | MACH_SEND_TIMEOUT,
        static_cast< mach_msg_size_t >( message_size ),
        0U,
        MACH_PORT_NULL,
        0U,
        MACH_PORT_NULL
    );
    switch ( result )
    {
        case MACH_MSG_SUCCESS:
            return SendStatus::Sent;

        case MACH_SEND_INVALID_DEST:
            // The peer's port is gone, so this side's send right is now a dead name.
            return SendStatus::Closed;

        case MACH_SEND_TIMED_OUT:
        case MACH_SEND_INTERRUPTED:
            // The kernel gave the copied rights back; release them again.
            ::mach_msg_destroy( header );
            return LTB_MAKE_UNEXPECTED_ERROR(
                "the receiver isn't reading its messages (queue full)"
            );

        default:
            return tl::make_unexpected( mach_error( "mach_msg (send)", result ) );
    }
}

auto Channel::receive( std::span< std::byte > const buffer ) const -> utils::Result< ReceiveResult >
{
    LTB_CHECK_VALID( this->is_valid( ) );

    auto message = MessageBuffer{
        round_up_to_4( payload_offset( max_handles_per_message ) + buffer.size( ) )
        + sizeof( mach_msg_max_trailer_t )
    };

    while ( true )
    {
        auto const result = receive_message( receive_port_.get( ), message );
        if ( MACH_RCV_TIMED_OUT == result )
        {
            return ReceiveResult{ .status = ReceiveStatus::WouldBlock };
        }
        if ( MACH_RCV_TOO_LARGE == result )
        {
            return LTB_MAKE_UNEXPECTED_ERROR( "received a message larger than the buffer" );
        }
        if ( MACH_MSG_SUCCESS != result )
        {
            return tl::make_unexpected( mach_error( "mach_msg (receive)", result ) );
        }

        auto* const header = message.header( );

        if ( MACH_NOTIFY_NO_SENDERS == header->msgh_id )
        {
            // The peer is gone. Ask again, so the next receive reports it too (there are no
            // senders left, so the new notification is sent straight away).
            LTB_CHECK( request_no_senders_notification( receive_port_.get( ) ) );
            return ReceiveResult{ .status = ReceiveStatus::Closed };
        }
        if ( MACH_NOTIFY_SEND_ONCE == header->msgh_id )
        {
            continue; // An unused notification right was dropped (see above).
        }

        // Validate before taking ownership of anything. Invalid messages are destroyed,
        // which releases whatever rights they carry.
        auto const fail = [ header ]( char const* const reason ) -> utils::Result< ReceiveResult > {
            ::mach_msg_destroy( header );
            return LTB_MAKE_UNEXPECTED_ERROR( "received an invalid message: {}", reason );
        };

        if ( ( data_message_id != header->msgh_id )
             || ( 0U == ( header->msgh_bits & MACH_MSGH_BITS_COMPLEX ) ) )
        {
            return fail( "unexpected message type" );
        }
        auto const body         = message.read< mach_msg_body_t >( sizeof( mach_msg_header_t ) );
        auto const handle_count = std::size_t{ body.msgh_descriptor_count };
        if ( handle_count > max_handles_per_message )
        {
            return fail( "too many handles" );
        }
        if ( header->msgh_size < payload_offset( handle_count ) )
        {
            return fail( "too short" );
        }

        auto descriptors = std::array< mach_msg_port_descriptor_t, max_handles_per_message >{ };
        for ( auto i = 0UZ; i < handle_count; ++i )
        {
            descriptors[ i ] = message.read< mach_msg_port_descriptor_t >(
                descriptors_offset + ( i * sizeof( mach_msg_port_descriptor_t ) )
            );
            if ( !is_port_descriptor( descriptors[ i ], MACH_MSG_TYPE_PORT_SEND ) )
            {
                return fail( "a handle isn't a port send right" );
            }
        }

        auto const payload_size
            = std::size_t{ message.read< mach_msg_size_t >( payload_size_offset( handle_count ) ) };
        if ( payload_offset( handle_count ) + payload_size > header->msgh_size )
        {
            return fail( "payload size doesn't match the message size" );
        }
        if ( payload_size > buffer.size( ) )
        {
            return fail( "message is larger than the buffer" );
        }

        auto received = ReceiveResult{
            .status  = ReceiveStatus::Message,
            .message = { .size = payload_size },
        };
        if ( payload_size > 0U )
        {
            std::memcpy(
                buffer.data( ),
                message.bytes( ) + payload_offset( handle_count ),
                payload_size
            );
        }
        received.message.handles.reserve( handle_count );
        for ( auto i = 0UZ; i < handle_count; ++i )
        {
            received.message.handles.emplace_back( descriptors[ i ].name );
        }
        return received;
    }
}

// ---------------------------------------------------------------------------------------
// ChannelListener
// ---------------------------------------------------------------------------------------

ChannelListener::ChannelListener( MachReceiveRight port )
    : port_( std::move( port ) )
{
}

auto ChannelListener::listen( ChannelAddress const& address ) -> utils::Result< ChannelListener >
{
    LTB_CHECK_VALID( !address.empty( ) );
    LTB_CHECK_VALID(
        address.size( ) < sizeof( name_t ),
        "launchd service names must be shorter than 128 characters"
    );

    // Registers a new port under the name, if no other process has.
    auto       port   = mach_port_t{ MACH_PORT_NULL };
    auto const result = ::bootstrap_check_in( bootstrap_port, address.c_str( ), &port );
    // launchd says "service active" if this process registered the name, but "permission
    // denied" if another process did. Check whether the name is taken to tell them apart.
    auto const name_is_taken = [ &address ]( ) {
        auto existing = mach_port_t{ MACH_PORT_NULL };
        if ( BOOTSTRAP_SUCCESS
             != ::bootstrap_look_up( bootstrap_port, address.c_str( ), &existing ) )
        {
            return false;
        }
        ::mach_port_deallocate( ::mach_task_self( ), existing );
        return true;
    };
    if ( ( BOOTSTRAP_SUCCESS != result )
         && ( ( BOOTSTRAP_SERVICE_ACTIVE == result ) || name_is_taken( ) ) )
    {
        return LTB_MAKE_UNEXPECTED_ERROR( "Another process is already listening at '{}'", address );
    }
    if ( BOOTSTRAP_SUCCESS != result )
    {
        return LTB_MAKE_UNEXPECTED_ERROR(
            "Couldn't listen at '{}': bootstrap_check_in failed ({})",
            address,
            ::bootstrap_strerror( result )
        );
    }
    auto listener = ChannelListener{ MachReceiveRight{ port } };

    auto limits = mach_port_limits_t{ .mpl_qlimit = MACH_PORT_QLIMIT_LARGE };
    if ( auto const limit_result = ::mach_port_set_attributes(
             ::mach_task_self( ),
             port,
             MACH_PORT_LIMITS_INFO,
             reinterpret_cast< mach_port_info_t >( &limits ),
             MACH_PORT_LIMITS_INFO_COUNT
         );
         KERN_SUCCESS != limit_result )
    {
        return tl::make_unexpected( mach_error( "mach_port_set_attributes", limit_result ) );
    }

    return listener;
}

auto ChannelListener::is_valid( ) const -> bool
{
    return port_.is_valid( );
}

auto ChannelListener::accept( ) -> utils::Result< std::optional< Channel > >
{
    LTB_CHECK_VALID( this->is_valid( ) );

    auto buffer = MessageBuffer{ sizeof( ReceivedConnectMessage ) };

    // Any process can look the name up and send something, so skip anything that isn't a
    // valid connect message.
    while ( true )
    {
        auto const result = receive_message( port_.get( ), buffer );
        if ( MACH_RCV_TIMED_OUT == result )
        {
            return std::nullopt;
        }
        if ( MACH_RCV_TOO_LARGE == result )
        {
            continue; // Not a connect message (and already destroyed).
        }
        if ( MACH_MSG_SUCCESS != result )
        {
            return tl::make_unexpected( mach_error( "mach_msg (accept)", result ) );
        }

        auto* const header  = buffer.header( );
        auto const  message = buffer.read< ConnectMessage >( 0U );

        auto const is_connect_message
            = ( connect_message_id == header->msgh_id )
           && ( 0U != ( header->msgh_bits & MACH_MSGH_BITS_COMPLEX ) )
           && ( sizeof( ConnectMessage ) == header->msgh_size )
           && ( 2U == message.body.msgh_descriptor_count )
           && is_port_descriptor( message.client_port, MACH_MSG_TYPE_PORT_SEND )
           && is_port_descriptor( message.listener_port, MACH_MSG_TYPE_PORT_RECEIVE );

        if ( !is_connect_message )
        {
            ::mach_msg_destroy( header );
            continue;
        }

        auto receive_port = MachReceiveRight{ message.listener_port.name };
        auto peer_port    = MachSendRight{ message.client_port.name };

        // If the client has already gone, this is sent straight away, and the first receive
        // reports the connection as closed (after any messages it sent).
        LTB_CHECK( request_no_senders_notification( receive_port.get( ) ) );

        return Channel{ std::move( receive_port ), std::move( peer_port ) };
    }
}

} // namespace ltb::ipc
