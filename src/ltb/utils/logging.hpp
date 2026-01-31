// ///////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2022 Logan Barnes - All Rights Reserved
// ///////////////////////////////////////////////////////////////////////////////////////
#pragma once

// project
#include "result.hpp"

namespace ltb::utils
{

/// \brief Attempts to set the log level using a string instead of
///        the spdlog::level::log_level enum. For example, "debug"
///        can be used instead of spdlog::level::debug. This function
///        can be used to set the log level from command line inputs.
auto try_setting_log_level( std::string const& log_level ) -> utils::Result< void >;

} // namespace ltb::utils
