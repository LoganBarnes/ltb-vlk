// ///////////////////////////////////////////////////////////////////////////////////////
// A Logan Thomas Barnes project
// ///////////////////////////////////////////////////////////////////////////////////////
#pragma once

// standard
#include <concepts>
#include <cstdint>

/// Defined on platforms that have backends for `ipc::Channel` and `vlk::SharedImage`.
#if defined( __linux__ ) || defined( _WIN32 ) || defined( __APPLE__ )
#define LTB_IPC_SUPPORTED 1
#endif

namespace ltb::ipc
{

/// \brief What every platform's native handle type must provide: unique ownership of an OS
///        resource that can be passed to another process over a Channel.
template < typename Handle >
concept IsNativeHandle = std::default_initializable< Handle > && std::movable< Handle >
                      && !std::copyable< Handle > && requires( Handle handle ) {
                             { handle.get( ) };
                             { handle.is_valid( ) } -> std::same_as< bool >;
                             { handle.release( ) };
                             { handle.reset( ) } -> std::same_as< void >;
                         };

#if defined( __linux__ )

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

#elif defined( _WIN32 )

/// \brief Owns a Windows `HANDLE` and closes it (CloseHandle) when destroyed.
///
/// Stored as `void*` (which is what `HANDLE` is) so this header doesn't need <windows.h>.
/// `INVALID_HANDLE_VALUE` is treated the same as a null handle.
class WinHandle
{
public:
    WinHandle( ) = default;
    explicit WinHandle( void* handle );
    ~WinHandle( );

    WinHandle( WinHandle const& )                    = delete;
    auto operator=( WinHandle const& ) -> WinHandle& = delete;

    WinHandle( WinHandle&& other ) noexcept;
    auto operator=( WinHandle&& other ) noexcept -> WinHandle&;

    [[nodiscard( "Const getter" )]]
    auto get( ) const -> void*;

    [[nodiscard( "Const getter" )]]
    auto is_valid( ) const -> bool;

    /// \brief Gives up ownership without closing the handle.
    auto release( ) -> void*;

    auto reset( ) -> void;

private:
    void* handle_ = nullptr;
};

#elif defined( __APPLE__ )

/// \brief Owns one user reference to a Mach port send right, and releases it
///        (mach_port_deallocate) when destroyed.
///
/// Stored as `std::uint32_t` (which is what `mach_port_t` is) so this header doesn't need
/// the Mach headers. `MACH_PORT_NULL` (0) and `MACH_PORT_DEAD` are treated as invalid.
class MachSendRight
{
public:
    MachSendRight( ) = default;
    explicit MachSendRight( std::uint32_t port );
    ~MachSendRight( );

    MachSendRight( MachSendRight const& )                    = delete;
    auto operator=( MachSendRight const& ) -> MachSendRight& = delete;

    MachSendRight( MachSendRight&& other ) noexcept;
    auto operator=( MachSendRight&& other ) noexcept -> MachSendRight&;

    [[nodiscard( "Const getter" )]]
    auto get( ) const -> std::uint32_t;

    [[nodiscard( "Const getter" )]]
    auto is_valid( ) const -> bool;

    /// \brief Gives up ownership without releasing the right.
    auto release( ) -> std::uint32_t;

    auto reset( ) -> void;

private:
    std::uint32_t port_ = 0U;
};

#endif

#if defined( __linux__ )

/// \brief An owned OS handle that can be sent to another process.
///
/// - Linux: a file descriptor, passed with SCM_RIGHTS.
/// - Windows: an NT `HANDLE`, which the Channel duplicates into the receiving process with
///   `DuplicateHandle`.
/// - macOS: a Mach port send right (e.g. an IOSurface's port), passed in a Mach message.
using NativeHandle = FileDescriptor;

#elif defined( _WIN32 )

using NativeHandle = WinHandle;

#elif defined( __APPLE__ )

using NativeHandle = MachSendRight;

#endif

#if defined( LTB_IPC_SUPPORTED )
static_assert( IsNativeHandle< NativeHandle > );
#endif

} // namespace ltb::ipc
