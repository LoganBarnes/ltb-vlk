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
/// Backends (each in `src/ltb/ipc/channel_<platform>.cpp`):
///
///   Linux    Unix domain socket (SOCK_SEQPACKET); handles are fds sent with SCM_RIGHTS.
///   Windows  Not implemented yet. Plan: a message-mode named pipe. A sent handle is
///            duplicated into the receiving process with DuplicateHandle (the peer's process
///            id comes from GetNamedPipeClientProcessId / GetNamedPipeServerProcessId).
///   macOS    Not implemented yet. Plan: Mach messages (or XPC) carrying Mach port rights.
///
/// To add a platform: implement this header's functions in a new channel_<platform>.cpp,
/// define `NativeHandle` for it in native_handle.hpp, and add the file to CMakeLists.txt.
namespace ltb::ipc
{

/// \brief Where a ChannelListener listens. A filesystem path on Linux; a pipe name such as
///        `\\.\pipe\name` on Windows.
using ChannelAddress = std::string;

/// \brief A per-user address for `name` that other processes of the same user can find.
///        Linux: `$XDG_RUNTIME_DIR/<name>.sock` (or a per-user path in /tmp).
auto default_channel_address( std::string_view name ) -> ChannelAddress;

#if defined( __linux__ )

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
    explicit Channel( UnixSocket socket );

    UnixSocket socket_ = { };
};

class ChannelListener
{
public:
    /// \brief Starts listening at `address`. On Linux, a stale socket file left by a crashed
    ///        process is replaced, and the file is removed when the listener is destroyed.
    static auto listen( ChannelAddress const& address ) -> utils::Result< ChannelListener >;

    ChannelListener( ) = default;

    [[nodiscard( "Const getter" )]]
    auto is_valid( ) const -> bool;

    /// \brief Accepts a pending connection, or returns `std::nullopt` if there isn't one.
    auto accept( ) const -> utils::Result< std::optional< Channel > >;

private:
    explicit ChannelListener( UnixSocket socket );

    UnixSocket socket_ = { };
};

#endif

} // namespace ltb::ipc
