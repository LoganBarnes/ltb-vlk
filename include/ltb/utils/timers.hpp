// ///////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) Logan Barnes - All Rights Reserved
// ///////////////////////////////////////////////////////////////////////////////////////
#pragma once

// project
#include "duration.hpp"

// standard
#include <chrono>
#include <functional>

namespace ltb::utils
{

class Timer
{
public:
    Timer( );

    auto start( ) -> void;
    auto duration_since_start( ) -> Duration;

private:
    std::chrono::time_point< std::chrono::steady_clock > start_time_;
};

class ScopedTimer
{
public:
    using Callback = std::function< void( Duration ) >;

    explicit ScopedTimer( Callback callback );
    ~ScopedTimer( );

private:
    Timer    timer_;
    Callback callback_;
};

} // namespace ltb::utils
