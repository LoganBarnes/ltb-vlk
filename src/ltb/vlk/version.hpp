// ///////////////////////////////////////////////////////////////////////////////////////
// A Logan Thomas Barnes project
// ///////////////////////////////////////////////////////////////////////////////////////
#pragma once

// project
#include "ltb/vlk/vulkan.hpp"

#define VLK_MAKE_VERSION( major, minor, patch )                                                    \
    ( ( ( uint32_t{ major } ) << 22U ) | ( ( uint32_t{ minor } ) << 12U ) | ( uint32_t{ patch } ) )

#define VLK_MAKE_API_VERSION( variant, major, minor, patch )                                       \
    ( ( ( uint32_t{ variant } ) << 29U ) | ( ( uint32_t{ major } ) << 22U )                        \
      | ( ( uint32_t{ minor } ) << 12U ) | ( uint32_t{ patch } ) )

#if defined( VK_API_VERSION_1_4 )
#define VLK_API_VERSION_1_4 VLK_MAKE_API_VERSION( 0, 1, 3, 0 )
#endif
