// ///////////////////////////////////////////////////////////////////////////////////////
// A Logan Thomas Barnes project
// ///////////////////////////////////////////////////////////////////////////////////////
#include "ltb/ipc/native_handle.hpp"

// system
#include <windows.h>

// standard
#include <utility>

namespace ltb::ipc
{

WinHandle::WinHandle( void* const handle )
    // Some APIs return INVALID_HANDLE_VALUE instead of null on failure. Store both as null.
    : handle_( ( INVALID_HANDLE_VALUE == handle ) ? nullptr : handle )
{
}

WinHandle::~WinHandle( )
{
    this->reset( );
}

WinHandle::WinHandle( WinHandle&& other ) noexcept
    : handle_( other.release( ) )
{
}

auto WinHandle::operator=( WinHandle&& other ) noexcept -> WinHandle&
{
    if ( this != &other )
    {
        this->reset( );
        handle_ = other.release( );
    }
    return *this;
}

auto WinHandle::get( ) const -> void*
{
    return handle_;
}

auto WinHandle::is_valid( ) const -> bool
{
    return nullptr != handle_;
}

auto WinHandle::release( ) -> void*
{
    return std::exchange( handle_, nullptr );
}

auto WinHandle::reset( ) -> void
{
    if ( this->is_valid( ) )
    {
        ::CloseHandle( handle_ );
        handle_ = nullptr;
    }
}

} // namespace ltb::ipc
