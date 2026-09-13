// ///////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2022 Logan Barnes - All Rights Reserved
// ///////////////////////////////////////////////////////////////////////////////////////
#include "ltb/utils/logging.hpp"

// external
#include <magic_enum.hpp>
#include <spdlog/spdlog.h>

// standard
#include <ranges>

namespace ltb::utils
{

auto try_setting_log_level( std::string const& log_level ) -> utils::Result< void >
{

    // log_levels -> {{level::debug, "debug"}, {level::trace, "trace"}, ... etc.}
    auto const log_levels = magic_enum::enum_entries< spdlog::level::level_enum >( )
                          | std::views::filter( []( auto const& value_and_name ) {
                                // remove the 'n_levels' enum from the list.
                                return value_and_name.first != spdlog::level::n_levels;
                            } )
                          | std::ranges::to< std::vector >( );

    auto log_level_specified = false;

    for ( auto const& [ value, name ] : log_levels )
    {
        if ( value == spdlog::level::n_levels )
        {
            continue;
        }
        if ( log_level == name )
        {
            spdlog::set_level( value );
            log_level_specified = true;
            spdlog::info( "Log level: {}", name );
        }
    }

    if ( !log_level_specified )
    {
        auto const log_levels_list
            = log_levels
            // {{level::debug, "debug"},{level::trace, "trace"},...} => {"debug", "trace",...}
            | std::views::transform( []( auto const& value_and_name ) {
                  //
                  return std::string( value_and_name.second );
              } )
            // {"debug", "trace",...} => "debug\ntrace\n...";
            | std::views::join_with( '\n' ) //
            | std::ranges::to< std::string >( );

        return LTB_MAKE_UNEXPECTED_ERROR(
            "Unrecognized log level: '{}'\n"
            "Available options are:\n"
            "{}",
            log_level,
            log_levels_list
        );
    }

    return success( );
}

} // namespace ltb::utils
