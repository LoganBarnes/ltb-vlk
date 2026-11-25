// ///////////////////////////////////////////////////////////////////////////////////////
// A Logan Thomas Barnes project
// ///////////////////////////////////////////////////////////////////////////////////////
#pragma once

// project
#include "ltb/utils/result.hpp"

// standard
#include <cstddef>
#include <filesystem>
#include <fstream>
#include <type_traits>
#include <vector>

namespace ltb::utils
{

/// \brief Reads a whole file into a vector of `T`. If the file size isn't a multiple of
///        `sizeof( T )`, the last element is zero-padded.
template < typename T >
    requires std::is_trivially_copyable_v< T >
auto get_binary_file_contents( std::filesystem::path const& file_path )
    -> Result< std::vector< T > >
{
    auto file = std::ifstream( file_path, std::ios::ate | std::ios::binary );

    if ( !file.is_open( ) )
    {
        return LTB_MAKE_UNEXPECTED_ERROR( "Failed to open file '{}'", file_path.string( ) );
    }

    auto const end_position = file.tellg( );
    if ( end_position < 0 )
    {
        return LTB_MAKE_UNEXPECTED_ERROR( "Failed to get the size of '{}'", file_path.string( ) );
    }
    auto const file_size = static_cast< std::size_t >( end_position );

    // Round up so a partial trailing element still fits.
    auto buffer = std::vector< T >( ( file_size + sizeof( T ) - 1U ) / sizeof( T ) );

    file.seekg( 0 );
    file.read(
        reinterpret_cast< char* >( buffer.data( ) ),
        static_cast< std::streamsize >( file_size )
    );
    if ( static_cast< std::size_t >( file.gcount( ) ) != file_size )
    {
        return LTB_MAKE_UNEXPECTED_ERROR(
            "Read {} of {} bytes from '{}'",
            file.gcount( ),
            file_size,
            file_path.string( )
        );
    }

    return buffer;
}

template < typename T >
    requires std::is_trivially_copyable_v< T >
auto get_binary_files_contents( std::vector< std::filesystem::path > const& file_paths )
    -> Result< std::vector< std::vector< T > > >
{

    auto buffers = std::vector< std::vector< T > >{ };
    buffers.reserve( file_paths.size( ) );

    for ( auto const& file_path : file_paths )
    {
        if ( auto buffer = get_binary_file_contents< T >( file_path ) )
        {
            buffers.emplace_back( std::move( buffer.value( ) ) );
        }
        else
        {
            return tl::make_unexpected( buffer.error( ) );
        }
    }

    return buffers;
}

} // namespace ltb::utils
