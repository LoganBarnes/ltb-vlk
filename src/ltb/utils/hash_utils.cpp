// ///////////////////////////////////////////////////////////////////////////////////////
// A Logan Thomas Barnes project
// ///////////////////////////////////////////////////////////////////////////////////////
#include "ltb/utils/hash_utils.hpp"

// standard
#include <random>

namespace ltb::utils
{

auto string_seed_to_uint( std::string const& str ) -> uint32
{
    auto seed_seq = std::seed_seq( str.begin( ), str.end( ) );
    auto seed     = std::vector< uint32 >( 1UL );
    seed_seq.generate( seed.begin( ), seed.end( ) );
    return seed.back( );
}

} // namespace ltb::utils
