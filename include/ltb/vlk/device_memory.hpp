// ///////////////////////////////////////////////////////////////////////////////////////
// A Logan Thomas Barnes project
// ///////////////////////////////////////////////////////////////////////////////////////
#pragma once

// project
#include "ltb/utils/result.hpp"
#include "ltb/utils/types.hpp"
#include "ltb/vlk/fwd.hpp"
#include "ltb/vlk/vulkan.hpp"

namespace ltb::vlk
{

struct DeviceMemorySettings
{

    vk::DeviceSize allocation_size   = 0UZ;
    uint32         memory_type_index = 0U;
};

struct MemoryRange
{
    vk::DeviceSize size   = 0UZ;
    vk::DeviceSize offset = 0UZ;
};

class DeviceMemory
{
public:
    explicit( false ) DeviceMemory( Device& device );
    ~DeviceMemory( );

    // No copy
    DeviceMemory( DeviceMemory const& )                    = delete;
    auto operator=( DeviceMemory const& ) -> DeviceMemory& = delete;

    // Move construction only (the device is a reference, so it can't be reassigned).
    DeviceMemory( DeviceMemory&& other ) noexcept;
    auto operator=( DeviceMemory&& ) noexcept -> DeviceMemory& = delete;

    auto initialize( DeviceMemorySettings settings ) -> utils::Result< void >;

    [[nodiscard( "Const getter" )]]
    auto is_initialized( ) const -> bool;

    [[nodiscard( "Const getter" )]]
    auto get( ) const -> vk::DeviceMemory const&;
    auto get( ) -> vk::DeviceMemory&;

    auto reset( ) -> void;

    [[nodiscard( "Const getter" )]]
    auto settings( ) const -> DeviceMemorySettings const&;

    [[nodiscard( "Const getter" )]]
    auto size( ) const -> vk::DeviceSize const&;

    auto map( ) -> utils::Result< uint8* >;
    auto map( MemoryRange const& range ) -> utils::Result< uint8* >;
    auto map( MemoryRange const& range, vk::MemoryMapFlags flags ) -> utils::Result< uint8* >;
    /// \brief Unmaps the memory if it's mapped. Does nothing otherwise.
    auto unmap( ) -> void;

private:
    Device& device_;

    DeviceMemorySettings   settings_      = { };
    vk::UniqueDeviceMemory device_memory_ = { };

    bool mapped_ = false;
};

struct MemoryLayout
{
    vk::DeviceSize             total_size = 0UZ;
    std::vector< MemoryRange > ranges     = { };
};

auto compute_memory_layout( std::vector< vk::MemoryRequirements > const& requirements )
    -> MemoryLayout;

struct MakeCopyRegion
{
    auto operator( )( MemoryRange const& range ) const -> vk::BufferCopy;
};

} // namespace ltb::vlk
