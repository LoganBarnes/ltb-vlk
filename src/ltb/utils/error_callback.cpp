// ///////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2022 Logan Barnes - All Rights Reserved
// ///////////////////////////////////////////////////////////////////////////////////////
#include "error_callback.hpp"

#include <spdlog/spdlog.h>

namespace ltb::utils
{

auto log_error( Error const& error ) -> void
{
    if ( error.severity( ) == Error::Severity::Warning )
    {
        spdlog::warn( "{}", error.debug_error_message( ) );
    }
    else
    {
        spdlog::error( "{}", error.debug_error_message( ) );
    }
}

auto invoke_if_non_null( ErrorCallback const& callback, Error error ) -> void
{
    if ( callback )
    {
        callback( std::move( error ) );
    }
}

} // namespace ltb::utils
