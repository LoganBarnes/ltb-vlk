// ///////////////////////////////////////////////////////////////////////////////////////
// A Logan Thomas Barnes project
// ///////////////////////////////////////////////////////////////////////////////////////
#pragma once

// project
#include "ltb/ipc/message.hpp"
#include "ltb/ipc/native_handle.hpp"
#include "ltb/utils/result.hpp"

#if defined( __linux__ )
#include "ltb/ipc/unix_socket.hpp"
#elif defined( __APPLE__ )
#include "ltb/ipc/mach_port.hpp"
#endif

#if defined( _WIN32 )
#include <memory>
#endif

// standard
#include <cstddef>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <type_traits>

/// A platform-neutral connection between processes that sends whole messages along with
/// native handles (`NativeHandle`). All operations are non-blocking.
///
/// Backends (each in `src/ltb/ipc/<platform>/`):
///
///   Linux    Unix domain socket (SOCK_SEQPACKET); handles are fds sent with SCM_RIGHTS.
///   Windows  A message-mode named pipe. A sent handle is duplicated into the receiving
///            process with DuplicateHandle (the peer's process id comes from
///            GetNamedPipeClientProcessId / GetNamedPipeServerProcessId), and the duplicate's
///            value is sent in the message.
///   macOS    Mach messages carrying port send rights. The listener registers its port with
///            launchd (bootstrap_check_in) under the address, and a client finds it with
///            bootstrap_look_up. Each side of a connection has its own receive port, and a
///            no-senders notification on it reports that the peer has gone.
///
/// To add a platform: implement this header's functions in src/ltb/ipc/<platform>/, define
/// `NativeHandle` and `LTB_IPC_SUPPORTED` for it in native_handle.hpp, and select the
/// directory in CMakeLists.txt.
namespace ltb::ipc
{

/// \brief Where a ChannelListener listens. A filesystem path on Linux; a pipe name such as
///        `\\.\pipe\name` on Windows; a launchd service name on macOS.
using ChannelAddress = std::string;

/// \brief A per-user address for `name` that other processes of the same user can find.
///        Linux: `$XDG_RUNTIME_DIR/<name>.sock` (or a per-user path in /tmp).
///        Windows: `\\.\pipe\<name>-<session id>`.
///        macOS: `ltb.ipc.<name>` (launchd service names are already per user).
auto default_channel_address( std::string_view name ) -> ChannelAddress;

#if defined( LTB_IPC_SUPPORTED )

class Channel
{
public:
    /// \brief Connects to a ChannelListener another process is listening on.
    static auto connect( ChannelAddress const& address ) -> utils::Result< Channel >;

    Channel( ) = default;

    [[nodiscard( "Const getter" )]]
    auto is_valid( ) const -> bool;

    /// \brief Sends one message, plus optional handles. The receiver gets its own copies;
    ///        the caller keeps ownership of `handles`. A closed connection is reported as
    ///        `SendStatus::Closed` rather than an error.
    auto
    send( std::span< std::byte const > bytes, std::span< NativeHandle const > handles = { } ) const
        -> utils::Result< SendStatus >;

    /// \brief Receives one message, and any handles sent with it, without blocking.
    auto receive( std::span< std::byte > buffer ) const -> utils::Result< ReceiveResult >;

    /// \brief Sends a trivially copyable struct as one message.
    template < typename T >
        requires std::is_trivially_copyable_v< T >
    auto send_value( T const& value, std::span< NativeHandle const > handles = { } ) const
        -> utils::Result< SendStatus >
    {
        return this->send( std::as_bytes( std::span{ &value, 1U } ), handles );
    }

private:
    friend class ChannelListener;

#if defined( __linux__ )
    explicit Channel( UnixSocket socket );

    UnixSocket socket_ = { };
#elif defined( _WIN32 )
    Channel( WinHandle pipe, WinHandle peer_process );

    WinHandle pipe_         = { }; ///< The connected pipe (opened for overlapped I/O)
    WinHandle peer_process_ = { }; ///< The other process, opened to duplicate handles into
#elif defined( __APPLE__ )
    Channel( MachReceiveRight receive_port, MachSendRight peer_port );

    MachReceiveRight receive_port_ = { }; ///< Where the peer's messages arrive
    MachSendRight    peer_port_    = { }; ///< The peer's receive port
#endif
};

class ChannelListener
{
public:
    /// \brief Starts listening at `address`. On Linux, a stale socket file left by a crashed
    ///        process is replaced, and the file is removed when the listener is destroyed.
    ///        On macOS, launchd drops the name when the listener is destroyed or its process
    ///        exits.
    static auto listen( ChannelAddress const& address ) -> utils::Result< ChannelListener >;

#if !defined( _WIN32 )
    ChannelListener( ) = default;
#endif

    [[nodiscard( "Const getter" )]]
    auto is_valid( ) const -> bool;

    /// \brief Accepts a pending connection, or returns `std::nullopt` if there isn't one.
    auto accept( ) -> utils::Result< std::optional< Channel > >;

#if defined( _WIN32 )
    // Defined in the .cpp, where `PendingConnection` is complete. MSVC instantiates the
    // `unique_ptr` destructor in an inline default constructor too, so that is out of line.
    ChannelListener( );
    ~ChannelListener( );
    ChannelListener( ChannelListener&& ) noexcept;
    auto operator=( ChannelListener&& ) noexcept -> ChannelListener&;
#endif

private:
#if defined( __linux__ )
    explicit ChannelListener( UnixSocket socket );

    UnixSocket socket_ = { };
#elif defined( __APPLE__ )
    explicit ChannelListener( MachReceiveRight port );

    MachReceiveRight port_ = { }; ///< Registered with launchd under the listening address
#elif defined( _WIN32 )
    /// \brief A pipe instance waiting (with overlapped I/O) for the next client.
    struct PendingConnection;

    ChannelListener( ChannelAddress address, std::unique_ptr< PendingConnection > pending );

    /// \brief Creates a pipe instance and starts waiting (without blocking) for a client.
    static auto start_listening( ChannelAddress const& address, bool first_instance )
        -> utils::Result< std::unique_ptr< PendingConnection > >;

    ChannelAddress                       address_ = { };
    std::unique_ptr< PendingConnection > pending_ = nullptr;
#endif
};

#endif

} // namespace ltb::ipc
