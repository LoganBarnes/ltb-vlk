// ///////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2022 Logan Barnes - All Rights Reserved
// ///////////////////////////////////////////////////////////////////////////////////////
#include "ltb/utils/logging.hpp"

// project
#include "ltb/utils/string.hpp"

// external
#include <spdlog/spdlog.h>

namespace ltb::utils
{

auto try_setting_log_level( std::string const& log_level ) -> utils::Result< void >
{
    // Accepts spdlog's full names ("warning", "error") and short names ("warn", "err"),
    // in any case.
    auto const name  = to_lower_ascii( log_level );
    auto const level = spdlog::level::from_str( name );

    // `from_str` returns `off` for unrecognized names.
    if ( ( spdlog::level::off == level ) && ( "off" != name ) )
    {
        return LTB_MAKE_UNEXPECTED_ERROR(
            "Unrecognized log level: '{}'\n"
            "Available options are: trace, debug, info, warning (warn), error (err), critical, off",
            log_level
        );
    }

    spdlog::set_level( level );
    spdlog::info( "Log level: {}", spdlog::level::to_string_view( level ) );

    return success( );
}

} // namespace ltb::utils
