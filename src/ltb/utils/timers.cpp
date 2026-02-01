// ///////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2022 Logan Barnes - All Rights Reserved
// ///////////////////////////////////////////////////////////////////////////////////////
#include "ltb/utils/timers.hpp"

// standard
#include <iostream>

namespace ltb::utils
{

Timer::Timer( )
    : start_time_( std::chrono::steady_clock::now( ) )
{
}

auto Timer::start( ) -> void
{
    start_time_ = std::chrono::steady_clock::now( );
}

auto Timer::duration_since_start( ) -> Duration
{
    auto const time_now = std::chrono::steady_clock::now( );
    return time_now - start_time_;
}

ScopedTimer::ScopedTimer( Callback callback )
    : callback_( std::move( callback ) )
{
    timer_.start( );
}

ScopedTimer::~ScopedTimer( )
{
    if ( callback_ )
    {
        callback_( timer_.duration_since_start( ) );
    }
}

} // namespace ltb::utils
