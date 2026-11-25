// ///////////////////////////////////////////////////////////////////////////////////////
// A Logan Thomas Barnes project
// ///////////////////////////////////////////////////////////////////////////////////////
#pragma once

// project
#include "ltb/utils/result.hpp"

// standard
#include <cstddef>
#include <filesystem>
#include <optional>
#include <span>
#include <type_traits>
#include <utility>
#include <vector>

namespace ltb::ipc
{

/// \brief Owns a POSIX file descriptor and closes it when destroyed.
class FileDescriptor
{
public:
    FileDescriptor( ) = default;
    explicit FileDescriptor( int fd );
    ~FileDescriptor( );

    FileDescriptor( FileDescriptor const& )                    = delete;
    auto operator=( FileDescriptor const& ) -> FileDescriptor& = delete;

    FileDescriptor( FileDescriptor&& other ) noexcept;
    auto operator=( FileDescriptor&& other ) noexcept -> FileDescriptor&;

    [[nodiscard( "Const getter" )]]
    auto get( ) const -> int;

    [[nodiscard( "Const getter" )]]
    auto is_valid( ) const -> bool;

    /// \brief Gives up ownership without closing the descriptor.
    auto release( ) -> int;

    auto reset( ) -> void;

private:
    int fd_ = -1;
};

/// \brief What a non-blocking receive found.
struct ReceivedMessage
{
    /// \brief The number of bytes written into the receive buffer.
    std::size_t size = 0U;

    /// \brief File descriptors sent along with the message.
    std::vector< FileDescriptor > fds = { };
};

enum class ReceiveStatus
{
    Message, ///< A message was received.
    WouldBlock, ///< Nothing to receive right now.
    Closed, ///< The other end closed the connection.
};

enum class SendStatus
{
    Sent, ///< The message was sent.
    Closed, ///< The other end closed the connection, so nothing was sent.
};

struct ReceiveResult
{
    ReceiveStatus   status  = ReceiveStatus::WouldBlock;
    ReceivedMessage message = { };
};

/// \brief A non-blocking, message-based (SOCK_SEQPACKET) Unix domain socket that can also
///        pass file descriptors to another process.
class UnixSocket
{
public:
    /// \brief Listens for connections at `path`, replacing a stale socket file if there is one.
    ///        The file is removed when the listening socket is destroyed.
    static auto listen( std::filesystem::path path ) -> utils::Result< UnixSocket >;

    /// \brief Connects to a socket that another process is listening on.
    static auto connect( std::filesystem::path const& path ) -> utils::Result< UnixSocket >;

    /// \brief Two connected sockets (mostly useful for tests).
    static auto pair( ) -> utils::Result< std::pair< UnixSocket, UnixSocket > >;

    UnixSocket( ) = default;
    ~UnixSocket( );

    UnixSocket( UnixSocket const& )                    = delete;
    auto operator=( UnixSocket const& ) -> UnixSocket& = delete;

    UnixSocket( UnixSocket&& other ) noexcept;
    auto operator=( UnixSocket&& other ) noexcept -> UnixSocket&;

    [[nodiscard( "Const getter" )]]
    auto is_valid( ) const -> bool;

    [[nodiscard( "Const getter" )]]
    auto fd( ) const -> int;

    /// \brief Accepts a pending connection, or returns `std::nullopt` if there isn't one.
    auto accept( ) const -> utils::Result< std::optional< UnixSocket > >;

    /// \brief Sends one message, plus optional file descriptors. The descriptors are
    ///        duplicated into the receiving process; the caller still owns its copies.
    ///        A connection the other end has closed is reported as `SendStatus::Closed`,
    ///        not an error, since that's an expected way for a connection to end.
    auto send( std::span< std::byte const > bytes, std::span< int const > fds = { } ) const
        -> utils::Result< SendStatus >;

    /// \brief Receives one message without blocking. Messages longer than `buffer` are an error.
    auto receive( std::span< std::byte > buffer ) const -> utils::Result< ReceiveResult >;

    /// \brief Sends a trivially copyable struct as one message.
    template < typename T >
        requires std::is_trivially_copyable_v< T >
    auto send_value( T const& value, std::span< int const > fds = { } ) const
        -> utils::Result< SendStatus >
    {
        return this->send( std::as_bytes( std::span{ &value, 1U } ), fds );
    }

private:
    explicit UnixSocket( FileDescriptor fd, std::filesystem::path owned_path = { } );

    FileDescriptor        fd_         = { };
    std::filesystem::path owned_path_ = { };
};

/// \brief The largest number of file descriptors `UnixSocket::receive` accepts per message.
inline constexpr auto max_fds_per_message = 16UZ;

} // namespace ltb::ipc
