// ///////////////////////////////////////////////////////////////////////////////////////
// A Logan Thomas Barnes project
// ///////////////////////////////////////////////////////////////////////////////////////
#include "ltb/vlk/objs/vulkan_compute_pipeline.hpp"

// project
#include "ltb/vlk/check.hpp"
#include "ltb/vlk/objs/frame_info.hpp"

// external
#include <spdlog/spdlog.h>

namespace ltb::vlk::objs
{

VulkanComputePipeline::VulkanComputePipeline( VulkanGpu& gpu )
    : gpu_( gpu )
{
}

auto VulkanComputePipeline::initialize( VulkanComputePipelineSettings settings )
    -> utils::Result< void >
{
    if ( this->is_initialized( ) )
    {
        return utils::success( );
    }
    LTB_CHECK_VALID( gpu_.is_initialized( ) );
    LTB_CHECK_VALID( settings.descriptor_set_count > 0U );

    LTB_CHECK( shader_module_.initialize( std::move( settings.shader_module ) ) );

    LTB_CHECK( descriptor_set_layout_.initialize( {
        .bindings = std::move( settings.uniform_bindings ),
    } ) );

    LTB_CHECK( descriptor_sets_.initialize( {
        .layouts = {
            settings.descriptor_set_count,
            descriptor_set_layout_.get( ),
        },
    } ) );

    LTB_CHECK( pipeline_layout_.initialize( {
        .descriptor_set_layouts = { descriptor_set_layout_.get( ) },
        .push_constant_ranges   = std::move( settings.uniform_push_constants ),
    } ) );

    LTB_CHECK( pipeline_.initialize( ) );

    initialized_ = true;

    return utils::success( );
}

auto VulkanComputePipeline::is_initialized( ) const -> bool
{
    return initialized_;
}

auto VulkanComputePipeline::bind( vk::CommandBuffer const& command_buffer ) -> void
{
    command_buffer.bindPipeline( vk::PipelineBindPoint::eCompute, pipeline_.get( ) );
}

auto VulkanComputePipeline::bind_descriptor_sets( FrameInfo const& frame ) -> utils::Result< void >
{
    auto const descriptor_sets = descriptor_sets_.get( );
    LTB_CHECK_VALID( frame.frame_index < descriptor_sets.size( ) );

    constexpr auto bind_point = vk::PipelineBindPoint::eCompute;
    constexpr auto first_set  = 0U;
    frame.command_buffer.bindDescriptorSets(
        bind_point,
        pipeline_layout_.get( ),
        first_set,
        descriptor_sets[ frame.frame_index ],
        { }
    );

    return utils::success( );
}

auto VulkanComputePipeline::shader_module( ) const -> ShaderModule const&
{
    return shader_module_;
}

auto VulkanComputePipeline::shader_module( ) -> ShaderModule&
{
    return shader_module_;
}

auto VulkanComputePipeline::descriptor_set_layout( ) const -> DescriptorSetLayout const&
{
    return descriptor_set_layout_;
}

auto VulkanComputePipeline::descriptor_set_layout( ) -> DescriptorSetLayout&
{
    return descriptor_set_layout_;
}

auto VulkanComputePipeline::descriptor_sets( ) const -> DescriptorSets const&
{
    return descriptor_sets_;
}

auto VulkanComputePipeline::descriptor_sets( ) -> DescriptorSets&
{
    return descriptor_sets_;
}

auto VulkanComputePipeline::pipeline_layout( ) const -> PipelineLayout const&
{
    return pipeline_layout_;
}

auto VulkanComputePipeline::pipeline_layout( ) -> PipelineLayout&
{
    return pipeline_layout_;
}

auto VulkanComputePipeline::pipeline( ) const -> ComputePipeline const&
{
    return pipeline_;
}

auto VulkanComputePipeline::pipeline( ) -> ComputePipeline&
{
    return pipeline_;
}

} // namespace ltb::vlk::objs
