// ///////////////////////////////////////////////////////////////////////////////////////
// A Logan Thomas Barnes project
// ///////////////////////////////////////////////////////////////////////////////////////
#pragma once

// project
#include "ltb/utils/result.hpp"
#include "ltb/utils/types.hpp"
#include "ltb/vlk/fwd.hpp"
#include "ltb/vlk/vulkan.hpp"

// standard
#include <optional>

namespace ltb::vlk
{

struct DeviceMemorySettings
{
    vk::DeviceSize allocation_size   = 0UZ;
    uint32         memory_type_index = 0U;

    /// \brief Handle types the memory can be exported as (see `DeviceMemory::export_fd`).
    vk::ExternalMemoryHandleTypeFlags export_handle_types = { };

    /// \brief Imports memory exported by another device or process instead of allocating new
    ///        memory. `allocation_size` and `memory_type_index` must match the exporter's.
    ///        On success Vulkan owns the file descriptor; on failure the caller still owns it.
    std::optional< int > import_fd = std::nullopt;

    /// \brief Allocates the memory for this image alone. Some drivers require this for
    ///        exported or imported image memory, and both sides must agree, so it's
    ///        simplest to always set it when sharing image memory.
    vk::Image dedicated_image = nullptr;

    /// \brief Extra structs to chain onto `vk::MemoryAllocateInfo`, for platform-specific
    ///        options without a setting here (e.g. `vk::ImportMemoryWin32HandleInfoKHR`).
    void const* p_next = nullptr;
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

    /// \brief Returns a new file descriptor for this memory, which another device or process
    ///        can import (see `DeviceMemorySettings::import_fd`). The caller owns the result.
    ///        The memory must have been allocated with `export_handle_types` including `type`.
    auto export_fd(
        vk::ExternalMemoryHandleTypeFlagBits type = vk::ExternalMemoryHandleTypeFlagBits::eOpaqueFd
    ) const -> utils::Result< int >;

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
