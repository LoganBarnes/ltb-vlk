// ///////////////////////////////////////////////////////////////////////////////////////
// A Logan Thomas Barnes project
// ///////////////////////////////////////////////////////////////////////////////////////
#include "ltb/vlk/device_memory.hpp"

// project
#include "ltb/vlk/check.hpp"
#include "ltb/vlk/device.hpp"
#include "ltb/vlk/physical_device.hpp"

// external
#include <spdlog/spdlog.h>

// standard
#include <ranges>
#include <utility>

namespace ltb::vlk
{
namespace
{

struct AppendMemoryRange
{
    vk::DeviceSize& total_size;

    auto operator( )( vk::MemoryRequirements const& mem_req ) const -> MemoryRange
    {
        // Each resource must be aligned to its own requirement.
        auto offset = total_size;
        if ( 0U != mem_req.alignment )
        {
            auto const alignment_remainder = offset % mem_req.alignment;
            if ( 0U != alignment_remainder )
            {
                offset += mem_req.alignment - alignment_remainder;
            }
        }

        total_size = offset + mem_req.size;

        return MemoryRange{ .size = mem_req.size, .offset = offset };
    }
};

} // namespace

DeviceMemory::DeviceMemory( Device& device )
    : device_( device )
{
}

DeviceMemory::DeviceMemory( DeviceMemory&& other ) noexcept
    : device_( other.device_ )
    , settings_( std::exchange( other.settings_, { } ) )
    , device_memory_( std::move( other.device_memory_ ) )
    , mapped_( std::exchange( other.mapped_, false ) )
{
}

DeviceMemory::~DeviceMemory( )
{
    this->unmap( );
}

auto DeviceMemory::initialize( DeviceMemorySettings const settings ) -> utils::Result< void >
{
    if ( this->is_initialized( ) )
    {
        return utils::success( );
    }
    LTB_CHECK_VALID( device_.is_initialized( ) );

    auto alloc_info = vk::MemoryAllocateInfo{ }
                          .setAllocationSize( settings.allocation_size )
                          .setMemoryTypeIndex( settings.memory_type_index );

    // Optional structs are linked into the pNext chain only when used.
    auto export_info
        = vk::ExportMemoryAllocateInfo{ }.setHandleTypes( settings.export_handle_types );
    auto import_info = vk::ImportMemoryFdInfoKHR{ }
                           .setHandleType( vk::ExternalMemoryHandleTypeFlagBits::eOpaqueFd )
                           .setFd( settings.import_fd.value_or( -1 ) );
    auto dedicated_info = vk::MemoryDedicatedAllocateInfo{ }.setImage( settings.dedicated_image );

    auto const prepend = [ &alloc_info ]( auto& info ) -> void {
        info.pNext = alloc_info.pNext;
        alloc_info.setPNext( &info );
    };
    if ( settings.export_handle_types )
    {
        prepend( export_info );
    }
    if ( settings.import_fd.has_value( ) )
    {
        prepend( import_info );
    }
    if ( settings.dedicated_image )
    {
        prepend( dedicated_info );
    }

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
    this->unmap( );
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
    if ( this->mapped_ )
    {
        device_.get( ).unmapMemory( device_memory_.get( ) );
        mapped_ = false;
    }
}

auto DeviceMemory::export_fd( vk::ExternalMemoryHandleTypeFlagBits const type ) const
    -> utils::Result< int >
{
    LTB_CHECK_VALID( this->is_initialized( ) );
    LTB_CHECK_VALID(
        static_cast< bool >( settings_.export_handle_types & type ),
        "memory wasn't allocated with this export handle type"
    );

    // An extension function, so it isn't exported by the Vulkan loader and must be looked up.
    auto const get_memory_fd = reinterpret_cast< PFN_vkGetMemoryFdKHR >(
        device_.get( ).getProcAddr( "vkGetMemoryFdKHR" )
    );
    LTB_CHECK_VALID( get_memory_fd, "VK_KHR_external_memory_fd isn't enabled on the device" );

    auto const fd_info
        = vk::MemoryGetFdInfoKHR{ }.setMemory( device_memory_.get( ) ).setHandleType( type );

    auto       fd     = -1;
    auto const result = vk::Result{ get_memory_fd(
        device_.get( ),
        reinterpret_cast< VkMemoryGetFdInfoKHR const* >( &fd_info ),
        &fd
    ) };
    if ( vk::Result::eSuccess != result )
    {
        return LTB_MAKE_UNEXPECTED_ERROR( "vkGetMemoryFdKHR failed: {}", vk::to_string( result ) );
    }
    return fd;
}

auto compute_memory_layout( std::vector< vk::MemoryRequirements > const& requirements )
    -> MemoryLayout
{
    auto layout = MemoryLayout{ };

    layout.ranges = requirements | std::views::transform( AppendMemoryRange{ layout.total_size } )
                  | std::ranges::to< std::vector >( );

    return layout;
}

auto MakeCopyRegion::operator( )( MemoryRange const& range ) const -> vk::BufferCopy
{
    return vk::BufferCopy{ }.setSrcOffset( 0U ).setDstOffset( range.offset ).setSize( range.size );
}

} // namespace ltb::vlk
