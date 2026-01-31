// ///////////////////////////////////////////////////////////////////////////////////////
// A Logan Thomas Barnes project
// ///////////////////////////////////////////////////////////////////////////////////////
#pragma once

// project
#include "ltb/vlk/command_buffer.hpp"
#include "ltb/vlk/descriptor_pool.hpp"
#include "ltb/vlk/device.hpp"
#include "ltb/vlk/instance.hpp"
#include "ltb/vlk/physical_device.hpp"
#include "ltb/vlk/surface.hpp"
#include "ltb/window/fwd.hpp"

namespace ltb::vlk::objs
{

struct VulkanGpuSettings
{
    InstanceSettings       instance        = { };
    DeviceSettings         device          = { };
    DescriptorPoolSettings descriptor_pool = { };
};

class VulkanGpu
{
public:
    VulkanGpu( window::GlfwContext& glfw_context, window::GlfwWindow& glfw_window );

    auto initialize( VulkanGpuSettings settings ) -> utils::Result< void >;

    [[nodiscard( "Const getter" )]]
    auto is_initialized( ) const -> bool;

    [[nodiscard( "Const getter" )]]
    auto instance( ) const -> Instance const&;
    auto instance( ) -> Instance&;

    [[nodiscard( "Const getter" )]]
    auto surface( ) const -> Surface const&;
    auto surface( ) -> Surface&;

    [[nodiscard( "Const getter" )]]
    auto physical_device( ) const -> PhysicalDevice const&;
    auto physical_device( ) -> PhysicalDevice&;

    [[nodiscard( "Const getter" )]]
    auto device( ) const -> Device const&;
    auto device( ) -> Device&;

    [[nodiscard( "Const getter" )]]
    auto descriptor_pool( ) const -> DescriptorPool const&;
    auto descriptor_pool( ) -> DescriptorPool&;

private:
    window::GlfwContext& glfw_context_;
    window::GlfwWindow&  glfw_window_;

    Instance       instance_        = { };
    Surface        surface_         = { glfw_window_, instance_ };
    PhysicalDevice physical_device_ = { instance_ };
    Device         device_          = { physical_device_ };

    DescriptorPool descriptor_pool_ = { device_ };

    bool initialized_ = false;
};

} // namespace ltb::vlk::objs
