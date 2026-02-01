// ///////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) Logan Barnes - All Rights Reserved
// ///////////////////////////////////////////////////////////////////////////////////////
#pragma once

// external
#include <spdlog/fmt/fmt.h>

// standard
#include <filesystem>
#include <string>

///\brief Macro used to auto-fill line and file information when creating an error.
///
/// \code
/// ltb::utils::Error error = LTB_MAKE_ERROR("Bad thing happened!");
/// assert(error.severity() == ltb::utils::Error::Severity::Error);
/// \endcode
#define LTB_MAKE_ERROR( ... )                                                                      \
    ::ltb::utils::Error(                                                                           \
        { __FILE__, __LINE__ },                                                                    \
        ::ltb::utils::Error::Severity::Error,                                                      \
        fmt::format( __VA_ARGS__ )                                                                 \
    )

///\brief Macro used to auto-fill line and file information when creating a warning.
/// \code
/// ltb::utils::Error error = ROTOR_MAKE_WARNING("Not so bad thing happened!");
/// assert(error.severity() == ltb::utils::Error::Severity::Warning);
/// \endcode
#define LTB_MAKE_WARNING( ... )                                                                    \
    ::ltb::utils::Error(                                                                           \
        { __FILE__, __LINE__ },                                                                    \
        ::ltb::utils::Error::Severity::Warning,                                                    \
        ,                                                                                          \
        fmt::format( __VA_ARGS__ )                                                                 \
    )

namespace ltb::utils
{

struct SourceLocation
{
    std::filesystem::path filename;
    int                   line_number;

    SourceLocation( ) = delete;
    SourceLocation( std::filesystem::path file, int line );
};

///\brief A simple class used to pass error messages around.
class Error
{
public:
    enum class Severity
    {
        Error,
        Warning,
    };

    Error( ) = delete;
    explicit Error( SourceLocation source_location, Severity severity, std::string error_message );
    virtual ~Error( );

    // default copy
    Error( Error const& )                    = default;
    auto operator=( Error const& ) -> Error& = default;

    // default move
    Error( Error&& ) noexcept                    = default;
    auto operator=( Error&& ) noexcept -> Error& = default;

    [[nodiscard]] auto severity( ) const -> Severity const&;
    [[nodiscard]] auto error_message( ) const -> std::string const&;
    [[nodiscard]] auto debug_error_message( ) const -> std::string const&;
    [[nodiscard]] auto source_location( ) const -> SourceLocation const&;

    static auto append_message( Error const& error, std::string const& message ) -> Error;

    auto operator==( Error const& other ) const -> bool;
    auto operator!=( Error const& other ) const -> bool;

private:
    SourceLocation source_location_; ///< File and line number where error was created
    Severity       severity_; ///< The type of error (warning or error)
    std::string    error_message_; ///< The error message
    std::string
        debug_message_; ///< Error message with file and line number "[file:line] error message"
};

/// \brief An error with extra data pertaining to a specific type of error.
template < typename Context >
struct ContextError
{
    Error   error;
    Context context;

    [[nodiscard]] auto severity( ) const -> Error::Severity const&
    {
        return error.severity( );
    }

    [[nodiscard]] auto error_message( ) const -> std::string const&
    {
        return error.error_message( );
    }

    [[nodiscard]] auto debug_error_message( ) const -> std::string const&
    {
        return error.debug_error_message( );
    }

    [[nodiscard]] auto source_location( ) const -> SourceLocation const&
    {
        return error.source_location( );
    }

    ContextError( Error err, Context ctx )
        : error( std::move( err ) )
        , context( std::move( ctx ) )
    {
    }
};

template < typename Context >
auto make_context_error( Error error, Context context ) -> ContextError< Context >
{
    return { std::move( error ), std::move( context ) };
}

} // namespace ltb::utils
