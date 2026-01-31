// ///////////////////////////////////////////////////////////////////////////////////////
// A Logan Thomas Barnes project
// ///////////////////////////////////////////////////////////////////////////////////////
#include "ltb/vlk/fence.hpp"

// project
#include "ltb/vlk/check.hpp"
#include "ltb/vlk/device.hpp"

// external
#include <spdlog/spdlog.h>

namespace ltb::vlk
{

Fence::Fence( Device& device )
    : device_( device )
{
}

auto Fence::initialize( ) -> utils::Result< void >
{
    return this->initialize( settings_ );
}

auto Fence::initialize( FenceSettings const settings ) -> utils::Result< void >
{
    if ( this->is_initialized( ) )
    {
        return utils::success( );
    }
    LTB_CHECK_VALID( device_.is_initialized( ) );

    auto const fence_info = vk::FenceCreateInfo{ }.setFlags( settings_.flags );

    VK_CHECK( auto fence, device_.get( ).createFenceUnique( fence_info ) );
    spdlog::debug( "vk::createFenceUnique()" );

    settings_ = settings;
    fence_    = std::move( fence );

    return utils::success( );
}

auto Fence::is_initialized( ) const -> bool
{
    return nullptr != fence_.get( );
}

auto Fence::get( ) const -> vk::Fence const&
{
    return fence_.get( );
}

auto Fence::get( ) -> vk::Fence&
{
    return fence_.get( );
}

} // namespace ltb::vlk
