// ///////////////////////////////////////////////////////////////////////////////////////
// A Logan Thomas Barnes project
// ///////////////////////////////////////////////////////////////////////////////////////
// The Windows backend of Channel: message-mode named pipes. Handles can't be sent through
// a pipe directly, so the sender duplicates each one into the receiving process
// (DuplicateHandle) and sends the duplicates' values in a header before the payload.
//
// NOT YET BUILT OR TESTED ON WINDOWS. Written on Linux as a starting point.
#include "ltb/ipc/channel.hpp"

// system
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

// standard
#include <array>
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

namespace ltb::ipc
{
namespace
{

/// \brief The size of each pipe instance's in and out buffers, and of the largest message
///        (header included) a Channel sends or receives.
constexpr auto pipe_buffer_size = DWORD{ 64U * 1024U };

/// \brief Prefixed to every message. Holds the values of the handles the sender duplicated
///        into the receiver's process, which the receiver then owns.
struct WireHeader
{
    std::uint32_t                                        handle_count = 0U;
    std::uint32_t                                        payload_size = 0U;
    std::array< std::uint64_t, max_handles_per_message > handles      = { };
};

auto error_message( DWORD const error ) -> std::string
{
    char*      buffer = nullptr;
    auto const length = ::FormatMessageA(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        nullptr,
        error,
        0,
        reinterpret_cast< char* >( &buffer ),
        0,
        nullptr
    );
    auto message = std::string( buffer, length );
    ::LocalFree( buffer );

    while ( !message.empty( ) && ( ( '\r' == message.back( ) ) || ( '\n' == message.back( ) ) ) )
    {
        message.pop_back( );
    }
    return message + " (error " + std::to_string( error ) + ")";
}

auto last_error_message( ) -> std::string
{
    return error_message( ::GetLastError( ) );
}

/// \brief True for the errors a pipe reports once the other end has closed it.
auto is_disconnect( DWORD const error ) -> bool
{
    return ( ERROR_BROKEN_PIPE == error ) || ( ERROR_NO_DATA == error )
        || ( ERROR_PIPE_NOT_CONNECTED == error );
}

enum class Io
{
    Read,
    Write,
};

/// \brief Runs one overlapped read or write to completion and returns its error code
///        (ERROR_SUCCESS on success). Only called when it won't wait long: reads after
///        PeekNamedPipe reported data, and writes small enough to fit in the pipe buffer.
auto complete_io(
    HANDLE const pipe,
    Io const     io,
    void* const  data,
    DWORD const  size,
    DWORD&       transferred
) -> DWORD
{
    auto const event = WinHandle{ ::CreateEventW( nullptr, TRUE, FALSE, nullptr ) };
    if ( !event.is_valid( ) )
    {
        return ::GetLastError( );
    }

    auto overlapped   = OVERLAPPED{ };
    overlapped.hEvent = event.get( );

    auto const started = ( Io::Read == io ) ? ::ReadFile( pipe, data, size, nullptr, &overlapped )
                                            : ::WriteFile( pipe, data, size, nullptr, &overlapped );
    if ( !started && ( ERROR_IO_PENDING != ::GetLastError( ) ) )
    {
        return ::GetLastError( );
    }

    constexpr auto wait = TRUE;
    if ( !::GetOverlappedResult( pipe, &overlapped, &transferred, wait ) )
    {
        return ::GetLastError( );
    }
    return ERROR_SUCCESS;
}

/// \brief Opens the process on the other end of `pipe`, so handles can be duplicated into it.
auto open_peer_process( HANDLE const pipe, bool const is_server ) -> utils::Result< WinHandle >
{
    auto       process_id = ULONG{ 0U };
    auto const found      = is_server ? ::GetNamedPipeClientProcessId( pipe, &process_id )
                                      : ::GetNamedPipeServerProcessId( pipe, &process_id );
    if ( !found )
    {
        return LTB_MAKE_UNEXPECTED_ERROR(
            "Couldn't get the peer's process id: {}",
            last_error_message( )
        );
    }

    // SYNCHRONIZE lets send() check whether the peer has exited.
    auto process
        = WinHandle{ ::OpenProcess( PROCESS_DUP_HANDLE | SYNCHRONIZE, FALSE, process_id ) };
    if ( !process.is_valid( ) )
    {
        return LTB_MAKE_UNEXPECTED_ERROR(
            "Couldn't open process {} to share handles with it: {}",
            process_id,
            last_error_message( )
        );
    }
    return process;
}

auto has_exited( HANDLE const process ) -> bool
{
    return WAIT_OBJECT_0 == ::WaitForSingleObject( process, 0U );
}

} // namespace

auto default_channel_address( std::string_view const name ) -> ChannelAddress
{
    // Pipe names are machine-wide, so include the login session to keep users apart.
    auto session = DWORD{ 0U };
    ::ProcessIdToSessionId( ::GetCurrentProcessId( ), &session );
    return std::string{ R"(\\.\pipe\)" } + std::string{ name } + "-" + std::to_string( session );
}

// ---------------------------------------------------------------------------------------
// Channel
// ---------------------------------------------------------------------------------------

Channel::Channel( WinHandle pipe, WinHandle peer_process )
    : pipe_( std::move( pipe ) )
    , peer_process_( std::move( peer_process ) )
{
}

auto Channel::connect( ChannelAddress const& address ) -> utils::Result< Channel >
{
    auto pipe = WinHandle{ };

    constexpr auto attempts = 5;
    for ( auto attempt = 0; ( attempt < attempts ) && !pipe.is_valid( ); ++attempt )
    {
        pipe = WinHandle{ ::CreateFileA(
            address.c_str( ),
            GENERIC_READ | GENERIC_WRITE,
            0,
            nullptr,
            OPEN_EXISTING,
            FILE_FLAG_OVERLAPPED,
            nullptr
        ) };

        if ( !pipe.is_valid( ) )
        {
            if ( ERROR_PIPE_BUSY != ::GetLastError( ) )
            {
                return LTB_MAKE_UNEXPECTED_ERROR(
                    "Couldn't connect to '{}': {}",
                    address,
                    last_error_message( )
                );
            }
            // Every pipe instance is busy with another client. Wait for one to free up.
            constexpr auto wait_milliseconds = DWORD{ 1000U };
            ::WaitNamedPipeA( address.c_str( ), wait_milliseconds );
        }
    }
    LTB_CHECK_VALID( pipe.is_valid( ), "the pipe stayed busy" );

    // Read whole messages, matching the server's mode.
    auto mode = DWORD{ PIPE_READMODE_MESSAGE };
    if ( !::SetNamedPipeHandleState( pipe.get( ), &mode, nullptr, nullptr ) )
    {
        return LTB_MAKE_UNEXPECTED_ERROR(
            "SetNamedPipeHandleState failed: {}",
            last_error_message( )
        );
    }

    constexpr auto is_server = false;
    LTB_CHECK( auto peer_process, open_peer_process( pipe.get( ), is_server ) );

    return Channel{ std::move( pipe ), std::move( peer_process ) };
}

auto Channel::is_valid( ) const -> bool
{
    return pipe_.is_valid( );
}

auto Channel::send(
    std::span< std::byte const > const    bytes,
    std::span< NativeHandle const > const handles
) const -> utils::Result< SendStatus >
{
    LTB_CHECK_VALID( this->is_valid( ) );
    LTB_CHECK_VALID( !bytes.empty( ), "messages can't be empty" );
    LTB_CHECK_VALID( handles.size( ) <= max_handles_per_message );
    LTB_CHECK_VALID(
        ( sizeof( WireHeader ) + bytes.size( ) ) <= pipe_buffer_size,
        "message too large"
    );
    for ( auto const& handle : handles )
    {
        LTB_CHECK_VALID( handle.is_valid( ) );
    }

    if ( has_exited( peer_process_.get( ) ) )
    {
        return SendStatus::Closed;
    }

    auto header = WireHeader{
        .handle_count = static_cast< std::uint32_t >( handles.size( ) ),
        .payload_size = static_cast< std::uint32_t >( bytes.size( ) ),
    };

    // Duplicate each handle into the receiving process. If the message isn't delivered, the
    // duplicates are closed again so they don't leak in the other process.
    auto duplicates       = std::vector< HANDLE >{ };
    auto close_duplicates = [ this, &duplicates ]( ) -> void {
        for ( auto const duplicate : duplicates )
        {
            ::DuplicateHandle(
                peer_process_.get( ),
                duplicate,
                nullptr,
                nullptr,
                0,
                FALSE,
                DUPLICATE_CLOSE_SOURCE
            );
        }
    };

    for ( auto i = 0UZ; i < handles.size( ); ++i )
    {
        auto duplicate = HANDLE{ nullptr };
        if ( !::DuplicateHandle(
                 ::GetCurrentProcess( ),
                 handles[ i ].get( ),
                 peer_process_.get( ),
                 &duplicate,
                 0,
                 FALSE,
                 DUPLICATE_SAME_ACCESS
             ) )
        {
            auto const message = last_error_message( );
            close_duplicates( );
            return LTB_MAKE_UNEXPECTED_ERROR( "DuplicateHandle failed: {}", message );
        }
        duplicates.push_back( duplicate );
        header.handles[ i ]
            = static_cast< std::uint64_t >( reinterpret_cast< std::uintptr_t >( duplicate ) );
    }

    // Header and payload go in one write, so they arrive as one message.
    auto message = std::vector< std::byte >( sizeof( header ) + bytes.size( ) );
    std::memcpy( message.data( ), &header, sizeof( header ) );
    std::memcpy( message.data( ) + sizeof( header ), bytes.data( ), bytes.size( ) );

    auto       written = DWORD{ 0U };
    auto const error   = complete_io(
        pipe_.get( ),
        Io::Write,
        message.data( ),
        static_cast< DWORD >( message.size( ) ),
        written
    );
    if ( ERROR_SUCCESS != error )
    {
        close_duplicates( );
        if ( is_disconnect( error ) )
        {
            return SendStatus::Closed;
        }
        return LTB_MAKE_UNEXPECTED_ERROR( "WriteFile failed: {}", error_message( error ) );
    }
    LTB_CHECK_VALID( written == message.size( ) );

    return SendStatus::Sent;
}

auto Channel::receive( std::span< std::byte > const buffer ) const -> utils::Result< ReceiveResult >
{
    LTB_CHECK_VALID( this->is_valid( ) );

    // Check for a message first so the read below never blocks.
    auto available = DWORD{ 0U };
    if ( !::PeekNamedPipe( pipe_.get( ), nullptr, 0U, nullptr, &available, nullptr ) )
    {
        auto const error = ::GetLastError( );
        if ( is_disconnect( error ) )
        {
            return ReceiveResult{ .status = ReceiveStatus::Closed };
        }
        return LTB_MAKE_UNEXPECTED_ERROR( "PeekNamedPipe failed: {}", error_message( error ) );
    }
    if ( 0U == available )
    {
        return ReceiveResult{ .status = ReceiveStatus::WouldBlock };
    }

    // In message mode one read returns exactly one message.
    auto       message = std::vector< std::byte >( pipe_buffer_size );
    auto       read    = DWORD{ 0U };
    auto const error
        = complete_io( pipe_.get( ), Io::Read, message.data( ), pipe_buffer_size, read );
    if ( is_disconnect( error ) )
    {
        return ReceiveResult{ .status = ReceiveStatus::Closed };
    }
    if ( ERROR_SUCCESS != error )
    {
        return LTB_MAKE_UNEXPECTED_ERROR( "ReadFile failed: {}", error_message( error ) );
    }

    LTB_CHECK_VALID( read >= sizeof( WireHeader ), "received a malformed message" );
    auto header = WireHeader{ };
    std::memcpy( &header, message.data( ), sizeof( header ) );
    LTB_CHECK_VALID(
        header.handle_count <= max_handles_per_message,
        "received a malformed message"
    );
    LTB_CHECK_VALID(
        header.payload_size == ( read - sizeof( header ) ),
        "received a malformed message"
    );

    // Take ownership of the handles first, so they're closed on every path below.
    auto result = ReceiveResult{ .status = ReceiveStatus::Message };
    for ( auto i = 0U; i < header.handle_count; ++i )
    {
        result.message.handles.emplace_back(
            reinterpret_cast< void* >( static_cast< std::uintptr_t >( header.handles[ i ] ) )
        );
    }

    LTB_CHECK_VALID(
        header.payload_size <= buffer.size( ),
        "Received a message that didn't fit in the buffer"
    );
    std::memcpy( buffer.data( ), message.data( ) + sizeof( header ), header.payload_size );
    result.message.size = header.payload_size;

    return result;
}

// ---------------------------------------------------------------------------------------
// ChannelListener
// ---------------------------------------------------------------------------------------

struct ChannelListener::PendingConnection
{
    WinHandle  pipe       = { };
    WinHandle  event      = { };
    OVERLAPPED overlapped = { };
    bool       io_pending = false; ///< ConnectNamedPipe is still waiting for a client
    bool       connected  = false;

    PendingConnection( )                                             = default;
    PendingConnection( PendingConnection const& )                    = delete;
    auto operator=( PendingConnection const& ) -> PendingConnection& = delete;

    ~PendingConnection( )
    {
        // The kernel writes to `overlapped` until the pending ConnectNamedPipe finishes, so
        // cancel it and wait before the memory is freed.
        if ( io_pending )
        {
            ::CancelIoEx( pipe.get( ), &overlapped );
            auto ignored = DWORD{ 0U };
            ::GetOverlappedResult( pipe.get( ), &overlapped, &ignored, TRUE );
        }
    }
};

ChannelListener::ChannelListener(
    ChannelAddress                       address,
    std::unique_ptr< PendingConnection > pending
)
    : address_( std::move( address ) )
    , pending_( std::move( pending ) )
{
}

ChannelListener::~ChannelListener( )                                              = default;
ChannelListener::ChannelListener( ChannelListener&& ) noexcept                    = default;
auto ChannelListener::operator=( ChannelListener&& ) noexcept -> ChannelListener& = default;

auto ChannelListener::listen( ChannelAddress const& address ) -> utils::Result< ChannelListener >
{
    constexpr auto first_instance = true;
    LTB_CHECK( auto pending, start_listening( address, first_instance ) );
    return ChannelListener{ address, std::move( pending ) };
}

auto ChannelListener::is_valid( ) const -> bool
{
    return ( nullptr != pending_ ) && pending_->pipe.is_valid( );
}

auto ChannelListener::accept( ) -> utils::Result< std::optional< Channel > >
{
    LTB_CHECK_VALID( this->is_valid( ) );
    constexpr auto first_instance = false;

    auto& pending = *pending_;
    if ( !pending.connected )
    {
        auto       ignored = DWORD{ 0U };
        auto const no_wait = FALSE;
        auto const completed
            = ::GetOverlappedResult( pending.pipe.get( ), &pending.overlapped, &ignored, no_wait );
        if ( !completed )
        {
            auto const error = ::GetLastError( );
            if ( ERROR_IO_INCOMPLETE == error )
            {
                return std::nullopt; // No client yet.
            }

            // The connection failed (e.g. the client gave up). Start over with a new instance.
            pending.io_pending = false;
            LTB_CHECK( pending_, start_listening( address_, first_instance ) );
            if ( is_disconnect( error ) )
            {
                return std::nullopt;
            }
            return LTB_MAKE_UNEXPECTED_ERROR(
                "ConnectNamedPipe failed: {}",
                error_message( error )
            );
        }
        pending.io_pending = false;
        pending.connected  = true;
    }

    // Hand the connected instance to a Channel, and start waiting for the next client.
    LTB_CHECK( auto next, start_listening( address_, first_instance ) );
    auto pipe = std::move( pending_->pipe );
    pending_  = std::move( next );

    constexpr auto is_server = true;
    LTB_CHECK( auto peer_process, open_peer_process( pipe.get( ), is_server ) );

    return Channel{ std::move( pipe ), std::move( peer_process ) };
}

auto ChannelListener::start_listening( ChannelAddress const& address, bool const first_instance )
    -> utils::Result< std::unique_ptr< PendingConnection > >
{
    auto pending = std::make_unique< PendingConnection >( );

    // FILE_FLAG_FIRST_PIPE_INSTANCE makes a second listener on the same name fail clearly
    // instead of silently sharing it.
    auto const open_mode
        = DWORD{ PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED }
        | ( first_instance ? DWORD{ FILE_FLAG_FIRST_PIPE_INSTANCE } : DWORD{ 0U } );
    pending->pipe = WinHandle{ ::CreateNamedPipeA(
        address.c_str( ),
        open_mode,
        PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT | PIPE_REJECT_REMOTE_CLIENTS,
        PIPE_UNLIMITED_INSTANCES,
        pipe_buffer_size,
        pipe_buffer_size,
        0U,
        nullptr
    ) };
    if ( !pending->pipe.is_valid( ) )
    {
        auto const error = ::GetLastError( );
        if ( first_instance && ( ERROR_ACCESS_DENIED == error ) )
        {
            return LTB_MAKE_UNEXPECTED_ERROR(
                "Another process is already listening at '{}'",
                address
            );
        }
        return LTB_MAKE_UNEXPECTED_ERROR(
            "CreateNamedPipe for '{}' failed: {}",
            address,
            error_message( error )
        );
    }

    pending->event = WinHandle{ ::CreateEventW( nullptr, TRUE, FALSE, nullptr ) };
    if ( !pending->event.is_valid( ) )
    {
        return LTB_MAKE_UNEXPECTED_ERROR( "CreateEvent failed: {}", last_error_message( ) );
    }
    pending->overlapped.hEvent = pending->event.get( );

    if ( ::ConnectNamedPipe( pending->pipe.get( ), &pending->overlapped ) )
    {
        pending->connected = true;
        return pending;
    }

    switch ( auto const error = ::GetLastError( ) )
    {
        case ERROR_IO_PENDING:
            pending->io_pending = true; // Waiting for a client.
            break;
        case ERROR_PIPE_CONNECTED:
            pending->connected = true; // A client connected before ConnectNamedPipe was called.
            break;
        default:
            return LTB_MAKE_UNEXPECTED_ERROR(
                "ConnectNamedPipe failed: {}",
                error_message( error )
            );
    }
    return pending;
}

} // namespace ltb::ipc
