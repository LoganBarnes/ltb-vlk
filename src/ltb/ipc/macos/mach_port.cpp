// ///////////////////////////////////////////////////////////////////////////////////////
// A Logan Thomas Barnes project
// ///////////////////////////////////////////////////////////////////////////////////////
#include "ltb/ipc/mach_port.hpp"

// system
#include <mach/mach.h>

// standard
#include <type_traits>
#include <utility>

namespace ltb::ipc
{

static_assert( std::is_same_v< mach_port_t, std::uint32_t > );

// ---------------------------------------------------------------------------------------
// MachSendRight
// ---------------------------------------------------------------------------------------

MachSendRight::MachSendRight( std::uint32_t const port )
    : port_( port )
{
}

MachSendRight::~MachSendRight( )
{
    this->reset( );
}

MachSendRight::MachSendRight( MachSendRight&& other ) noexcept
    : port_( other.release( ) )
{
}

auto MachSendRight::operator=( MachSendRight&& other ) noexcept -> MachSendRight&
{
    if ( this != &other )
    {
        this->reset( );
        port_ = other.release( );
    }
    return *this;
}

auto MachSendRight::get( ) const -> std::uint32_t
{
    return port_;
}

auto MachSendRight::is_valid( ) const -> bool
{
    return MACH_PORT_VALID( port_ );
}

auto MachSendRight::release( ) -> std::uint32_t
{
    return std::exchange( port_, MACH_PORT_NULL );
}

auto MachSendRight::reset( ) -> void
{
    // A send right whose port was destroyed turns into a dead name, which is released the
    // same way.
    if ( MACH_PORT_NULL != port_ )
    {
        if ( MACH_PORT_DEAD != port_ )
        {
            ::mach_port_deallocate( ::mach_task_self( ), port_ );
        }
        port_ = MACH_PORT_NULL;
    }
}

// ---------------------------------------------------------------------------------------
// MachReceiveRight
// ---------------------------------------------------------------------------------------

auto MachReceiveRight::allocate( ) -> utils::Result< MachReceiveRight >
{
    auto       port   = mach_port_t{ MACH_PORT_NULL };
    auto const result = ::mach_port_allocate( ::mach_task_self( ), MACH_PORT_RIGHT_RECEIVE, &port );
    if ( KERN_SUCCESS != result )
    {
        return LTB_MAKE_UNEXPECTED_ERROR(
            "mach_port_allocate failed: {}",
            ::mach_error_string( result )
        );
    }
    auto receive_right = MachReceiveRight{ port };

    // The default limit is only 5 queued messages.
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
        return LTB_MAKE_UNEXPECTED_ERROR(
            "mach_port_set_attributes failed: {}",
            ::mach_error_string( limit_result )
        );
    }
    return receive_right;
}

MachReceiveRight::MachReceiveRight( std::uint32_t const port )
    : port_( port )
{
}

MachReceiveRight::~MachReceiveRight( )
{
    this->reset( );
}

MachReceiveRight::MachReceiveRight( MachReceiveRight&& other ) noexcept
    : port_( other.release( ) )
{
}

auto MachReceiveRight::operator=( MachReceiveRight&& other ) noexcept -> MachReceiveRight&
{
    if ( this != &other )
    {
        this->reset( );
        port_ = other.release( );
    }
    return *this;
}

auto MachReceiveRight::get( ) const -> std::uint32_t
{
    return port_;
}

auto MachReceiveRight::is_valid( ) const -> bool
{
    return MACH_PORT_VALID( port_ );
}

auto MachReceiveRight::make_send_right( ) const -> utils::Result< MachSendRight >
{
    LTB_CHECK_VALID( this->is_valid( ) );
    auto const result
        = ::mach_port_insert_right( ::mach_task_self( ), port_, port_, MACH_MSG_TYPE_MAKE_SEND );
    if ( KERN_SUCCESS != result )
    {
        return LTB_MAKE_UNEXPECTED_ERROR(
            "mach_port_insert_right failed: {}",
            ::mach_error_string( result )
        );
    }
    return MachSendRight{ port_ };
}

auto MachReceiveRight::release( ) -> std::uint32_t
{
    return std::exchange( port_, MACH_PORT_NULL );
}

auto MachReceiveRight::reset( ) -> void
{
    if ( this->is_valid( ) )
    {
        // Destroys the port (and any queued messages). Senders' rights become dead names.
        ::mach_port_mod_refs( ::mach_task_self( ), port_, MACH_PORT_RIGHT_RECEIVE, -1 );
    }
    port_ = MACH_PORT_NULL;
}

} // namespace ltb::ipc
