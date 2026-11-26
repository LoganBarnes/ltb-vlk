// ///////////////////////////////////////////////////////////////////////////////////////
// A Logan Thomas Barnes project
// ///////////////////////////////////////////////////////////////////////////////////////
#pragma once

#if defined( __APPLE__ )

// project
#include "ltb/ipc/native_handle.hpp"
#include "ltb/utils/result.hpp"

// standard
#include <cstdint>

namespace ltb::ipc
{

/// \brief Owns a Mach port receive right, and destroys it when destroyed. Messages sent to
///        the port queue up until they're received through this right. Part of the macOS
///        backend of `Channel`.
///
/// Stored as `std::uint32_t` (which is what `mach_port_t` is) so this header doesn't need
/// the Mach headers.
class MachReceiveRight
{
public:
    /// \brief A new port, with room for a generous number of queued messages so senders
    ///        rarely find the queue full.
    static auto allocate( ) -> utils::Result< MachReceiveRight >;

    MachReceiveRight( ) = default;
    explicit MachReceiveRight( std::uint32_t port );
    ~MachReceiveRight( );

    MachReceiveRight( MachReceiveRight const& )                    = delete;
    auto operator=( MachReceiveRight const& ) -> MachReceiveRight& = delete;

    MachReceiveRight( MachReceiveRight&& other ) noexcept;
    auto operator=( MachReceiveRight&& other ) noexcept -> MachReceiveRight&;

    [[nodiscard( "Const getter" )]]
    auto get( ) const -> std::uint32_t;

    [[nodiscard( "Const getter" )]]
    auto is_valid( ) const -> bool;

    /// \brief A new send right to this port, which can be given to another process.
    [[nodiscard( "Const computation" )]]
    auto make_send_right( ) const -> utils::Result< MachSendRight >;

    /// \brief Gives up ownership without destroying the right.
    auto release( ) -> std::uint32_t;

    auto reset( ) -> void;

private:
    std::uint32_t port_ = 0U;
};

} // namespace ltb::ipc

#endif // defined( __APPLE__ )
