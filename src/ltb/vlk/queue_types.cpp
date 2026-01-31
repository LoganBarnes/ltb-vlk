// ///////////////////////////////////////////////////////////////////////////////////////
// A Logan Thomas Barnes project
// ///////////////////////////////////////////////////////////////////////////////////////
#include "ltb/vlk/queue_types.hpp"

// project
#include "ltb/vlk/check.hpp"

// external
#include <magic_enum.hpp>
#include <range/v3/range/conversion.hpp>
#include <range/v3/view/filter.hpp>
#include <range/v3/view/remove_if.hpp>
#include <range/v3/view/transform.hpp>
#include <spdlog/spdlog.h>

namespace ltb::vlk
{
namespace
{

struct ContainedBy
{
    QueueFamilyMap const& queue_families;

    auto operator( )( auto const& type ) const
    {
        return queue_families.contains( type );
    }
};

auto all_queue_families_present(
    QueueFamilyMap const&           queue_families,
    std::vector< QueueType > const& expected_types
) -> utils::Result< QueueFamilyMap >
{
    if ( auto const unsupported_types
         = expected_types | ranges::views::remove_if( ContainedBy{ queue_families } )
         | ranges::views::transform( magic_enum::enum_name< QueueType > )
         | ranges::to< std::vector< std::string > >( );
         !unsupported_types.empty( ) )
    {
        return LTB_MAKE_UNEXPECTED_ERROR(
            "Queue types not supported:\n{}",
            fmt::join( unsupported_types, "\n" )
        );
    }
    return queue_families;
}

} // namespace

auto to_queue_type( vk::QueueFlagBits const& queue_type ) -> QueueType
{
    switch ( queue_type )
    {
        using enum vk::QueueFlagBits;
        using enum QueueType;
        case eGraphics:
            return Graphics;
        case eCompute:
            return Compute;
        case eTransfer:
            return Transfer;
        case eSparseBinding:
            return SparseBinding;
        case eProtected:
            return Protected;
        default:
            break;
    }
    return QueueType::Unknown;
}

struct ToQueueType
{
    auto operator( )( vk::QueueFlagBits const& queue_type ) const -> QueueType
    {
        return to_queue_type( queue_type );
    }
};

struct NotUnknownQueueType
{
    auto operator( )( QueueType const& type ) const -> bool
    {
        return type != QueueType::Unknown;
    }
};

auto build_queue_family_map(
    vk::PhysicalDevice const&               physical_device,
    std::vector< vk::QueueFlagBits > const& types,
    vk::SurfaceKHR const&                   surface
) -> utils::Result< QueueFamilyMap >
{
    auto expected_types = types | ranges::views::transform( ToQueueType{ } )
                        | ranges::views::filter( NotUnknownQueueType{ } )
                        | ranges::to< std::vector >( );
    if ( surface )
    {
        expected_types.push_back( QueueType::Surface );
    }

    auto       queue_family_map = QueueFamilyMap{ };
    auto const queue_families   = physical_device.getQueueFamilyProperties( );

    auto result = utils::Result< QueueFamilyMap >{ queue_family_map };

    auto const queue_family_count = queue_families.size( );
    for ( auto i = QueueIndex{ 0 }; i < queue_family_count; ++i )
    {
        if ( surface )
        {
            VK_CHECK(
                auto const surface_support,
                physical_device.getSurfaceSupportKHR( i, surface )
            );
            if ( surface_support )
            {
                queue_family_map[ QueueType::Surface ] = i;
            }
        }

        for ( auto const& type : types )
        {
            if ( queue_families[ i ].queueFlags & type )
            {
                queue_family_map[ to_queue_type( type ) ] = i;
            }
        }

        if ( ( result = all_queue_families_present( queue_family_map, expected_types ) ) )
        {
            return result;
        }
    }

    return result;
}

} // namespace ltb::vlk
