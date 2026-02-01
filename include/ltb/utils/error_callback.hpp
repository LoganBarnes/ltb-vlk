// ///////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2022 Logan Barnes - All Rights Reserved
// ///////////////////////////////////////////////////////////////////////////////////////
#pragma once

// project
#include "error.hpp"

// standard
#include <functional>
#include <stdexcept>

namespace ltb::utils
{

using ErrorCallback = std::function< void( Error ) >;

constexpr auto ignore_errors = nullptr;

template < typename E = std::runtime_error >
auto throw_error( Error const& error ) -> void
{
    throw E( error.debug_error_message( ) );
}

auto log_error( Error const& error ) -> void;

auto invoke_if_non_null( ErrorCallback const& callback, Error error ) -> void;

} // namespace ltb::utils
