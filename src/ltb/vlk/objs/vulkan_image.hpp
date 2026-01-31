// ///////////////////////////////////////////////////////////////////////////////////////
// A Logan Thomas Barnes project
// ///////////////////////////////////////////////////////////////////////////////////////
#pragma once

// project
#include "ltb/vlk/device_memory.hpp"
#include "ltb/vlk/image.hpp"
#include "ltb/vlk/objs/vulkan_gpu.hpp"

namespace ltb::vlk::objs
{

struct VulkanImageSettings
{
    ImageSettings           image             = { };
    vk::MemoryPropertyFlags memory_properties = vk::MemoryPropertyFlagBits::eDeviceLocal;
};

class VulkanImage
{
public:
    explicit( false ) VulkanImage( VulkanGpu& gpu );

    auto initialize( VulkanImageSettings settings ) -> utils::Result< void >;

    [[nodiscard( "Const getter" )]]
    auto is_initialized( ) const -> bool;

    auto reset( ) -> void;

    [[nodiscard( "Const getter" )]]
    auto image( ) const -> Image const&;
    auto image( ) -> Image&;

    [[nodiscard( "Const getter" )]]
    auto memory( ) const -> DeviceMemory const&;
    auto memory( ) -> DeviceMemory&;

private:
    VulkanGpu& gpu_;

    Image        image_  = { gpu_.device( ) };
    DeviceMemory memory_ = { gpu_.device( ) };

    bool initialized_ = false;
};

} // namespace ltb::vlk::objs
