// ///////////////////////////////////////////////////////////////////////////////////////
// A Logan Thomas Barnes project
// ///////////////////////////////////////////////////////////////////////////////////////
#include "ltb/vlk/semaphore.hpp"

// project
#include "ltb/vlk/check.hpp"
#include "ltb/vlk/device.hpp"

// external
#include <spdlog/spdlog.h>

namespace ltb::vlk
{

Semaphore::Semaphore( Device& device )
    : device_( device )
{
}

auto Semaphore::initialize( ) -> utils::Result< void >
{
    return this->initialize( settings_ );
}

auto Semaphore::initialize( SemaphoreSettings const settings ) -> utils::Result< void >
{
    if ( this->is_initialized( ) )
    {
        return utils::success( );
    }
    LTB_CHECK_VALID( device_.is_initialized( ) );

    auto const semaphore_info = vk::SemaphoreCreateInfo{ }.setFlags( settings.flags );

    VK_CHECK( auto semaphore, device_.get( ).createSemaphoreUnique( semaphore_info ) );
    spdlog::debug( "vk::createSemaphoreUnique()" );

    settings_  = settings;
    semaphore_ = std::move( semaphore );

    return utils::success( );
}

auto Semaphore::is_initialized( ) const -> bool
{
    return nullptr != semaphore_.get( );
}

auto Semaphore::get( ) const -> vk::Semaphore const&
{
    return semaphore_.get( );
}

auto Semaphore::get( ) -> vk::Semaphore&
{
    return semaphore_.get( );
}

} // namespace ltb::vlk
