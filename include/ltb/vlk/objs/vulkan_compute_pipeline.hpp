// ///////////////////////////////////////////////////////////////////////////////////////
// A Logan Thomas Barnes project
// ///////////////////////////////////////////////////////////////////////////////////////
#pragma once

// project
#include "ltb/utils/types.hpp"
#include "ltb/vlk/compute_pipeline.hpp"
#include "ltb/vlk/descriptor_set_layout.hpp"
#include "ltb/vlk/descriptor_sets.hpp"
#include "ltb/vlk/objs/fwd.hpp"
#include "ltb/vlk/objs/vulkan_gpu.hpp"
#include "ltb/vlk/pipeline_layout.hpp"
#include "ltb/vlk/shader_module.hpp"

namespace ltb::vlk::objs
{

struct VulkanComputePipelineSettings
{
    ShaderModuleSettings shader_module = { };

    uint32 descriptor_set_count = 0U;

    std::vector< vk::DescriptorSetLayoutBinding > uniform_bindings       = { };
    std::vector< vk::PushConstantRange >          uniform_push_constants = { };
};

class VulkanComputePipeline
{
public:
    explicit( false ) VulkanComputePipeline( VulkanGpu& gpu );

    auto initialize( VulkanComputePipelineSettings settings ) -> utils::Result< void >;

    [[nodiscard( "Const getter" )]]
    auto is_initialized( ) const -> bool;

    auto bind( vk::CommandBuffer const& command_buffer ) -> void;

    auto bind_descriptor_sets( FrameInfo const& frame ) -> utils::Result< void >;

    [[nodiscard( "Cosnt getter" )]]
    auto shader_module( ) const -> ShaderModule const&;
    auto shader_module( ) -> ShaderModule&;

    [[nodiscard( "Cosnt getter" )]]
    auto descriptor_set_layout( ) const -> DescriptorSetLayout const&;
    auto descriptor_set_layout( ) -> DescriptorSetLayout&;

    [[nodiscard( "Cosnt getter" )]]
    auto descriptor_sets( ) const -> DescriptorSets const&;
    auto descriptor_sets( ) -> DescriptorSets&;

    [[nodiscard( "Cosnt getter" )]]
    auto pipeline_layout( ) const -> PipelineLayout const&;
    auto pipeline_layout( ) -> PipelineLayout&;

    [[nodiscard( "Cosnt getter" )]]
    auto pipeline( ) const -> ComputePipeline const&;
    auto pipeline( ) -> ComputePipeline&;

private:
    VulkanGpu& gpu_;

    ShaderModule shader_module_ = { gpu_.device( ) };

    DescriptorSetLayout descriptor_set_layout_ = { gpu_.device( ) };
    DescriptorSets      descriptor_sets_       = { gpu_.device( ), gpu_.descriptor_pool( ) };

    PipelineLayout pipeline_layout_ = { gpu_.device( ) };

    ComputePipeline pipeline_ = {
        gpu_.device( ),
        shader_module_,
        pipeline_layout_,
    };

    bool initialized_ = false;
};

} // namespace ltb::vlk::objs
