// ///////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) Logan Barnes - All Rights Reserved
// ///////////////////////////////////////////////////////////////////////////////////////
#include "error.hpp"

namespace ltb::utils
{

SourceLocation::SourceLocation( std::filesystem::path file, int line )
    : filename( std::move( file ) )
    , line_number( line )
{
}

namespace
{

///
/// \brief Prepend the file and line number to the message if they were specified.
///
auto create_debug_message( SourceLocation const& source_location, std::string const& error_message )
    -> std::string
{
    std::string result = "[" + source_location.filename.string( );

    if ( source_location.line_number >= 0 )
    {
        result += ':' + std::to_string( source_location.line_number );
    }

    result += "] " + error_message;
    return result;
}

} // namespace

Error::~Error( ) = default;

Error::Error( SourceLocation source_location, Severity severity, std::string error_message )
    : source_location_( std::move( source_location ) )
    , severity_( severity )
    , error_message_( std::move( error_message ) )
    , debug_message_( create_debug_message( source_location_, error_message_ ) )
{
}

auto Error::severity( ) const -> Error::Severity const&
{
    return severity_;
}

auto Error::error_message( ) const -> std::string const&
{
    return error_message_;
}

auto Error::debug_error_message( ) const -> std::string const&
{
    return debug_message_;
}

auto Error::source_location( ) const -> SourceLocation const&
{
    return source_location_;
}

auto Error::append_message( Error const& error, std::string const& message ) -> Error
{
    return Error( error.source_location_, error.severity_, error.error_message_ + " " + message );
}

auto Error::operator==( Error const& other ) const -> bool
{
    return debug_message_ == other.debug_message_;
}

auto Error::operator!=( Error const& other ) const -> bool
{
    return !( this->operator==( other ) );
}

} // namespace ltb::utils
