// ///////////////////////////////////////////////////////////////////////////////////////
// A Logan Thomas Barnes project
// ///////////////////////////////////////////////////////////////////////////////////////
#pragma once

// standard
#include <concepts>

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

/// \brief An owned OS handle that can be sent to another process.
///
/// - Linux: a file descriptor, passed with SCM_RIGHTS.
/// - Windows (not implemented yet): an NT `HANDLE`, which the Channel duplicates into the
///   receiving process with `DuplicateHandle`.
/// - macOS (not implemented yet): a Mach port send right, passed in a Mach/XPC message.
using NativeHandle = FileDescriptor;

static_assert( IsNativeHandle< NativeHandle > );

#endif

} // namespace ltb::ipc
