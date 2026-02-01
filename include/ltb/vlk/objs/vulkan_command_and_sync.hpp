// ///////////////////////////////////////////////////////////////////////////////////////
// A Logan Thomas Barnes project
// ///////////////////////////////////////////////////////////////////////////////////////
#pragma once

// project
#include "ltb/vlk/command_buffer.hpp"
#include "ltb/vlk/command_pool.hpp"
#include "ltb/vlk/fence.hpp"
#include "ltb/vlk/fwd.hpp"
#include "ltb/vlk/objs/frame_info.hpp"
#include "ltb/vlk/objs/vulkan_gpu.hpp"
#include "ltb/vlk/semaphore.hpp"

namespace ltb::vlk::objs
{

struct VulkanCommandAndSyncSettings
{
    uint32 frame_count = 0U;
    uint32 image_count = 0U;

    CommandPoolSettings command_pool = { };
};

enum class ResetCommandBuffer
{
    No,
    Yes,
};

struct FrameObjects
{
    vk::CommandBuffer command_buffer;
    vk::Fence         frame_fence;
    vk::Semaphore     frame_semaphore;
};

struct SemaphoreAndStage
{
    vk::Semaphore          semaphore = nullptr;
    vk::PipelineStageFlags stage     = vk::PipelineStageFlagBits::eNone;
};

class VulkanCommandAndSync
{
public:
    explicit( false ) VulkanCommandAndSync( VulkanGpu& gpu );

    auto initialize( VulkanCommandAndSyncSettings settings ) -> utils::Result< void >;

    [[nodiscard( "Const getter" )]]
    auto is_initialized( ) const -> bool;

    auto start_frame( Swapchain const& swapchain ) -> utils::Result< std::optional< FrameInfo > >;
    auto start_frame( Swapchain const& swapchain, ResetCommandBuffer reset_command_buffer )
        -> utils::Result< std::optional< FrameInfo > >;

    auto start_frame( ) -> utils::Result< std::optional< FrameInfo > >;
    auto start_frame( ResetCommandBuffer reset_command_buffer )
        -> utils::Result< std::optional< FrameInfo > >;

    auto get_frame_objects( ) -> FrameObjects;
    auto get_frame_objects( uint32 frame_index ) -> utils::Result< FrameObjects >;
    auto get_present_semaphore( FrameInfo const& frame ) -> utils::Result< vk::Semaphore >;

    auto end_frame( FrameInfo const& frame, vk::Queue const& submit_queue )
        -> utils::Result< void >;

    auto end_frame(
        FrameInfo const&                        frame,
        std::vector< SemaphoreAndStage > const& wait_until_signaled,
        std::vector< vk::Semaphore > const&     signal_when_finished,
        vk::Queue const&                        submit_queue
    ) -> utils::Result< void >;

    auto present_frame(
        FrameInfo const& frame,
        Swapchain const& swapchain,
        vk::Queue const& present_queue
    ) -> utils::Result< void >;

    auto increment_frame( ) -> void;

    [[nodiscard( "Const getter" )]] auto frame_index( ) const -> uint32;
    [[nodiscard( "Const getter" )]] auto previous_frame( ) const -> uint32;
    [[nodiscard( "Const getter" )]] auto compute_previous_frame( uint index ) const -> uint32;
    [[nodiscard( "Const getter" )]] auto compute_next_frame( uint index ) const -> uint32;

    [[nodiscard( "Const getter" )]]
    auto command_pool( ) const -> CommandPool const&;
    auto command_pool( ) -> CommandPool&;

    [[nodiscard( "Const getter" )]]
    auto command_buffers( ) const -> std::vector< CommandBuffer > const&;
    auto command_buffers( ) -> std::vector< CommandBuffer >&;

    [[nodiscard( "Const getter" )]]
    auto frame_fences( ) const -> std::vector< Fence > const&;
    auto frame_fences( ) -> std::vector< Fence >&;

    [[nodiscard( "Const getter" )]]
    auto frame_semaphores( ) const -> std::vector< Semaphore > const&;
    auto frame_semaphores( ) -> std::vector< Semaphore >&;

    [[nodiscard( "Const getter" )]]
    auto image_semaphores( ) const -> std::vector< Semaphore > const&;
    auto image_semaphores( ) -> std::vector< Semaphore >&;

private:
    VulkanGpu& gpu_;

    CommandPool command_pool_ = { gpu_.device( ), gpu_.physical_device( ) };

    std::vector< CommandBuffer > command_buffers_  = { };
    std::vector< Fence >         frame_fences_     = { };
    std::vector< Semaphore >     frame_semaphores_ = { };

    uint32 frame_index_ = 0U;

    std::vector< Semaphore > image_semaphores_ = { };

    bool initialized_ = false;
};

} // namespace ltb::vlk::objs
