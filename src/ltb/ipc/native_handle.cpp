// ///////////////////////////////////////////////////////////////////////////////////////
// A Logan Thomas Barnes project
// ///////////////////////////////////////////////////////////////////////////////////////
#include "ltb/ipc/native_handle.hpp"

// system
#include <unistd.h>

// standard
#include <utility>

namespace ltb::ipc
{

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

} // namespace ltb::ipc
