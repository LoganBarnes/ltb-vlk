// ///////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2022 Logan Barnes - All Rights Reserved
// ///////////////////////////////////////////////////////////////////////////////////////
#include "logging.hpp"

// external
#include <magic_enum.hpp>
#include <range/v3/range/conversion.hpp>
#include <range/v3/view/cache1.hpp>
#include <range/v3/view/filter.hpp>
#include <range/v3/view/join.hpp>
#include <range/v3/view/transform.hpp>
#include <spdlog/spdlog.h>

namespace ltb::utils
{

auto try_setting_log_level( std::string const& log_level ) -> utils::Result< void >
{

    // log_levels -> {{level::debug, "debug"}, {level::trace, "trace"}, ... etc.}
    auto const log_levels = magic_enum::enum_entries< spdlog::level::level_enum >( )
                          | ranges::views::filter( []( auto const& value_and_name ) {
                                // remove the 'n_levels' enum from the list.
                                return value_and_name.first != spdlog::level::n_levels;
                            } )
                          | ranges::to< std::vector >;

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
            | ranges::views::transform( []( auto const& value_and_name ) {
                  //
                  return std::string( value_and_name.second );
              } )
            | ranges::views::cache1
            // {"debug", "trace",...} => "debug\ntrace\n...";
            | ranges::views::join( '\n' ) //
            | ranges::to< std::string >;

        return LTB_MAKE_UNEXPECTED_ERROR(
            "Unrecognized log level: '{}'\n"
            "Available options are:\n"
            "{}",
            log_level,
            log_levels_list
        );
    }

    return utils::success( );
}

} // namespace ltb::utils
