// ///////////////////////////////////////////////////////////////////////////////////////
// A Logan Thomas Barnes project
// ///////////////////////////////////////////////////////////////////////////////////////
#include "ltb/vlk/device_memory.hpp"

// project
#include "ltb/vlk/check.hpp"
#include "ltb/vlk/device.hpp"
#include "ltb/vlk/physical_device.hpp"

// external
#include <range/v3/range/conversion.hpp>
#include <range/v3/view/transform.hpp>
#include <spdlog/spdlog.h>

namespace ltb::vlk
{
namespace
{

struct AppendMemoryRange
{
    vk::DeviceSize& total_size;

    auto operator( )( vk::MemoryRequirements const& mem_req ) const -> MemoryRange
    {
        auto const range = MemoryRange{ .size = mem_req.size, .offset = total_size };

        total_size += mem_req.size;
        auto const alignment_remainder = total_size % mem_req.alignment;
        total_size += mem_req.alignment - alignment_remainder;

        return range;
    }
};

} // namespace

DeviceMemory::DeviceMemory( Device& device )
    : device_( device )
{
}

DeviceMemory::~DeviceMemory( )
{
    if ( this->mapped_ )
    {
        device_.get( ).unmapMemory( device_memory_.get( ) );
    }
}

auto DeviceMemory::initialize( DeviceMemorySettings const settings ) -> utils::Result< void >
{
    if ( this->is_initialized( ) )
    {
        return utils::success( );
    }
    LTB_CHECK_VALID( device_.is_initialized( ) );

    auto const alloc_info = vk::MemoryAllocateInfo{ }
                                .setAllocationSize( settings.allocation_size )
                                .setMemoryTypeIndex( settings.memory_type_index );

    VK_CHECK( auto memory, device_.get( ).allocateMemoryUnique( alloc_info ) );
    spdlog::debug( "vk::allocateMemoryUnique()" );

    settings_      = settings;
    device_memory_ = std::move( memory );

    return utils::success( );
}

auto DeviceMemory::is_initialized( ) const -> bool
{
    return nullptr != device_memory_.get( );
}

auto DeviceMemory::get( ) const -> vk::DeviceMemory const&
{
    return device_memory_.get( );
}

auto DeviceMemory::get( ) -> vk::DeviceMemory&
{
    return device_memory_.get( );
}

auto DeviceMemory::reset( ) -> void
{
    if ( this->mapped_ )
    {
        device_.get( ).unmapMemory( device_memory_.get( ) );
        mapped_ = false;
    }
    device_memory_ = { };
    settings_      = { };
}

auto DeviceMemory::settings( ) const -> DeviceMemorySettings const&
{
    return settings_;
}

auto DeviceMemory::size( ) const -> vk::DeviceSize const&
{
    return settings_.allocation_size;
}

auto DeviceMemory::map( ) -> utils::Result< uint8* >
{
    return this->map(
        MemoryRange{
            .size   = settings_.allocation_size,
            .offset = 0U,
        }
    );
}

auto DeviceMemory::map( MemoryRange const& range ) -> utils::Result< uint8* >
{
    return this->map( range, vk::MemoryMapFlags{ } );
}

auto DeviceMemory::map( MemoryRange const& range, vk::MemoryMapFlags flags )
    -> utils::Result< uint8* >
{
    LTB_CHECK_VALID( this->is_initialized( ) );
    LTB_CHECK_VALID( !this->mapped_ );

    VK_CHECK(
        auto* const data,
        device_.get( ).mapMemory( device_memory_.get( ), range.offset, range.size, flags )
    );

    mapped_ = true;

    return static_cast< uint8* >( data );
}

auto DeviceMemory::unmap( ) -> void
{
    device_.get( ).unmapMemory( device_memory_.get( ) );
    mapped_ = false;
}

auto compute_memory_layout( std::vector< vk::MemoryRequirements > const& requirements )
    -> MemoryLayout
{
    auto layout = MemoryLayout{ };

    layout.ranges = requirements
                  | ranges::views::transform( AppendMemoryRange{ layout.total_size } )
                  | ranges::to< std::vector >( );

    return layout;
}

auto MakeCopyRegion::operator( )( MemoryRange const& range ) const -> vk::BufferCopy
{
    return vk::BufferCopy{ }.setSrcOffset( 0U ).setDstOffset( range.offset ).setSize( range.size );
}

} // namespace ltb::vlk
