// ///////////////////////////////////////////////////////////////////////////////////////
// A Logan Thomas Barnes project
// ///////////////////////////////////////////////////////////////////////////////////////
#include "ltb/ipc/unix_socket.hpp"

// system
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

// standard
#include <array>
#include <cerrno>
#include <cstring>

namespace ltb::ipc
{
namespace
{

auto errno_message( ) -> std::string
{
    return std::strerror( errno );
}

auto make_address( std::filesystem::path const& path ) -> utils::Result< sockaddr_un >
{
    auto address       = sockaddr_un{ };
    address.sun_family = AF_UNIX;

    auto const path_string = path.string( );
    if ( path_string.empty( ) || ( path_string.size( ) >= sizeof( address.sun_path ) ) )
    {
        return LTB_MAKE_UNEXPECTED_ERROR(
            "Socket path must be 1 to {} characters long: '{}'",
            sizeof( address.sun_path ) - 1U,
            path_string
        );
    }
    std::memcpy( address.sun_path, path_string.c_str( ), path_string.size( ) + 1U );
    return address;
}

auto make_socket( ) -> utils::Result< FileDescriptor >
{
    auto fd
        = FileDescriptor{ ::socket( AF_UNIX, SOCK_SEQPACKET | SOCK_NONBLOCK | SOCK_CLOEXEC, 0 ) };
    if ( !fd.is_valid( ) )
    {
        return LTB_MAKE_UNEXPECTED_ERROR( "socket() failed: {}", errno_message( ) );
    }
    return fd;
}

auto would_block( int const error ) -> bool
{
    return ( EAGAIN == error ) || ( EWOULDBLOCK == error );
}

} // namespace

// ---------------------------------------------------------------------------------------
// FileDescriptor
// ---------------------------------------------------------------------------------------

FileDescriptor::FileDescriptor( int const fd )
    : fd_( fd )
{
}

FileDescriptor::~FileDescriptor( )
{
    this->reset( );
}

FileDescriptor::FileDescriptor( FileDescriptor&& other ) noexcept
    : fd_( other.release( ) )
{
}

auto FileDescriptor::operator=( FileDescriptor&& other ) noexcept -> FileDescriptor&
{
    if ( this != &other )
    {
        this->reset( );
        fd_ = other.release( );
    }
    return *this;
}

auto FileDescriptor::get( ) const -> int
{
    return fd_;
}

auto FileDescriptor::is_valid( ) const -> bool
{
    return fd_ >= 0;
}

auto FileDescriptor::release( ) -> int
{
    return std::exchange( fd_, -1 );
}

auto FileDescriptor::reset( ) -> void
{
    if ( this->is_valid( ) )
    {
        ::close( fd_ );
        fd_ = -1;
    }
}

// ---------------------------------------------------------------------------------------
// UnixSocket
// ---------------------------------------------------------------------------------------

UnixSocket::UnixSocket( FileDescriptor fd, std::filesystem::path owned_path )
    : fd_( std::move( fd ) )
    , owned_path_( std::move( owned_path ) )
{
}

UnixSocket::~UnixSocket( )
{
    if ( !owned_path_.empty( ) )
    {
        auto error = std::error_code{ };
        std::filesystem::remove( owned_path_, error );
    }
}

UnixSocket::UnixSocket( UnixSocket&& other ) noexcept
    : fd_( std::move( other.fd_ ) )
    , owned_path_( std::exchange( other.owned_path_, { } ) )
{
}

auto UnixSocket::operator=( UnixSocket&& other ) noexcept -> UnixSocket&
{
    if ( this != &other )
    {
        // Run the destructor's cleanup for the current socket first.
        auto old    = UnixSocket{ std::move( fd_ ), std::exchange( owned_path_, { } ) };
        fd_         = std::move( other.fd_ );
        owned_path_ = std::exchange( other.owned_path_, { } );
    }
    return *this;
}

auto UnixSocket::listen( std::filesystem::path path ) -> utils::Result< UnixSocket >
{
    LTB_CHECK( auto const address, make_address( path ) );
    LTB_CHECK( auto fd, make_socket( ) );

    // A socket file left behind by a process that crashed would make bind() fail.
    if ( std::filesystem::is_socket( path ) )
    {
        auto error = std::error_code{ };
        std::filesystem::remove( path, error );
    }

    if ( 0
         != ::
             bind( fd.get( ), reinterpret_cast< sockaddr const* >( &address ), sizeof( address ) ) )
    {
        return LTB_MAKE_UNEXPECTED_ERROR(
            "bind() to '{}' failed: {}",
            path.string( ),
            errno_message( )
        );
    }

    constexpr auto backlog = 16;
    if ( 0 != ::listen( fd.get( ), backlog ) )
    {
        return LTB_MAKE_UNEXPECTED_ERROR( "listen() failed: {}", errno_message( ) );
    }

    return UnixSocket{ std::move( fd ), std::move( path ) };
}

auto UnixSocket::connect( std::filesystem::path const& path ) -> utils::Result< UnixSocket >
{
    LTB_CHECK( auto const address, make_address( path ) );
    LTB_CHECK( auto fd, make_socket( ) );

    // Connecting to a listening Unix socket completes immediately, even when non-blocking.
    if ( 0
         != ::connect(
             fd.get( ),
             reinterpret_cast< sockaddr const* >( &address ),
             sizeof( address )
         ) )
    {
        return LTB_MAKE_UNEXPECTED_ERROR(
            "connect() to '{}' failed: {}",
            path.string( ),
            errno_message( )
        );
    }

    return UnixSocket{ std::move( fd ) };
}

auto UnixSocket::pair( ) -> utils::Result< std::pair< UnixSocket, UnixSocket > >
{
    auto fds = std::array{ -1, -1 };
    if ( 0
         != ::socketpair( AF_UNIX, SOCK_SEQPACKET | SOCK_NONBLOCK | SOCK_CLOEXEC, 0, fds.data( ) ) )
    {
        return LTB_MAKE_UNEXPECTED_ERROR( "socketpair() failed: {}", errno_message( ) );
    }
    return std::pair{
        UnixSocket{ FileDescriptor{ fds[ 0 ] } },
        UnixSocket{ FileDescriptor{ fds[ 1 ] } },
    };
}

auto UnixSocket::is_valid( ) const -> bool
{
    return fd_.is_valid( );
}

auto UnixSocket::fd( ) const -> int
{
    return fd_.get( );
}

auto UnixSocket::accept( ) const -> utils::Result< std::optional< UnixSocket > >
{
    LTB_CHECK_VALID( this->is_valid( ) );

    auto fd
        = FileDescriptor{ ::accept4( fd_.get( ), nullptr, nullptr, SOCK_NONBLOCK | SOCK_CLOEXEC ) };
    if ( !fd.is_valid( ) )
    {
        if ( would_block( errno ) )
        {
            return std::nullopt;
        }
        return LTB_MAKE_UNEXPECTED_ERROR( "accept() failed: {}", errno_message( ) );
    }
    return UnixSocket{ std::move( fd ) };
}

auto UnixSocket::send(
    std::span< std::byte const > const bytes,
    std::span< int const > const       fds
) const -> utils::Result< SendStatus >
{
    LTB_CHECK_VALID( this->is_valid( ) );
    LTB_CHECK_VALID( !bytes.empty( ), "messages can't be empty" );
    LTB_CHECK_VALID( fds.size( ) <= max_fds_per_message );

    auto io_vector = iovec{
        .iov_base = const_cast< std::byte* >( bytes.data( ) ),
        .iov_len  = bytes.size( ),
    };

    auto message       = msghdr{ };
    message.msg_iov    = &io_vector;
    message.msg_iovlen = 1;

    // Ancillary data carrying the file descriptors (SCM_RIGHTS).
    alignas( cmsghdr ) auto control
        = std::array< std::byte, CMSG_SPACE( sizeof( int ) * max_fds_per_message ) >{ };
    if ( !fds.empty( ) )
    {
        auto const fds_size    = sizeof( int ) * fds.size( );
        message.msg_control    = control.data( );
        message.msg_controllen = CMSG_SPACE( fds_size );

        auto* const header = CMSG_FIRSTHDR( &message );
        header->cmsg_level = SOL_SOCKET;
        header->cmsg_type  = SCM_RIGHTS;
        header->cmsg_len   = CMSG_LEN( fds_size );
        std::memcpy( CMSG_DATA( header ), fds.data( ), fds_size );
    }

    // MSG_NOSIGNAL: report a closed connection as an error instead of raising SIGPIPE.
    auto const sent = ::sendmsg( fd_.get( ), &message, MSG_NOSIGNAL );
    if ( sent < 0 )
    {
        if ( ( EPIPE == errno ) || ( ECONNRESET == errno ) )
        {
            return SendStatus::Closed;
        }
        return LTB_MAKE_UNEXPECTED_ERROR( "sendmsg() failed: {}", errno_message( ) );
    }
    LTB_CHECK_VALID( static_cast< std::size_t >( sent ) == bytes.size( ) );

    return SendStatus::Sent;
}

auto UnixSocket::receive( std::span< std::byte > const buffer ) const
    -> utils::Result< ReceiveResult >
{
    LTB_CHECK_VALID( this->is_valid( ) );

    auto io_vector = iovec{
        .iov_base = buffer.data( ),
        .iov_len  = buffer.size( ),
    };

    alignas( cmsghdr ) auto control
        = std::array< std::byte, CMSG_SPACE( sizeof( int ) * max_fds_per_message ) >{ };

    auto message           = msghdr{ };
    message.msg_iov        = &io_vector;
    message.msg_iovlen     = 1;
    message.msg_control    = control.data( );
    message.msg_controllen = control.size( );

    auto const received = ::recvmsg( fd_.get( ), &message, MSG_CMSG_CLOEXEC );
    if ( received < 0 )
    {
        if ( would_block( errno ) )
        {
            return ReceiveResult{ .status = ReceiveStatus::WouldBlock };
        }
        if ( ECONNRESET == errno )
        {
            return ReceiveResult{ .status = ReceiveStatus::Closed };
        }
        return LTB_MAKE_UNEXPECTED_ERROR( "recvmsg() failed: {}", errno_message( ) );
    }

    // Take ownership of any received descriptors first so they're closed on every path.
    auto result = ReceiveResult{ .status = ReceiveStatus::Message };
    for ( auto* header = CMSG_FIRSTHDR( &message ); nullptr != header;
          header       = CMSG_NXTHDR( &message, header ) )
    {
        if ( ( SOL_SOCKET == header->cmsg_level ) && ( SCM_RIGHTS == header->cmsg_type ) )
        {
            auto const count = ( header->cmsg_len - CMSG_LEN( 0 ) ) / sizeof( int );
            for ( auto i = 0UZ; i < count; ++i )
            {
                auto fd = -1;
                std::memcpy( &fd, CMSG_DATA( header ) + ( i * sizeof( int ) ), sizeof( int ) );
                result.message.fds.emplace_back( fd );
            }
        }
    }

    if ( 0 == received )
    {
        // A zero-length message on a SOCK_SEQPACKET socket means the peer closed it.
        return ReceiveResult{ .status = ReceiveStatus::Closed };
    }
    if ( 0 != ( message.msg_flags & ( MSG_TRUNC | MSG_CTRUNC ) ) )
    {
        return LTB_MAKE_UNEXPECTED_ERROR( "Received a message that didn't fit in the buffer" );
    }

    result.message.size = static_cast< std::size_t >( received );
    return result;
}

} // namespace ltb::ipc
