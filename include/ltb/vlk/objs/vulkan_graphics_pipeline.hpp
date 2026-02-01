// ///////////////////////////////////////////////////////////////////////////////////////
// A Logan Thomas Barnes project
// ///////////////////////////////////////////////////////////////////////////////////////
#pragma once

// project
#include "ltb/utils/types.hpp"
#include "ltb/vlk/descriptor_set_layout.hpp"
#include "ltb/vlk/descriptor_sets.hpp"
#include "ltb/vlk/graphics_pipeline.hpp"
#include "ltb/vlk/objs/fwd.hpp"
#include "ltb/vlk/objs/vulkan_gpu.hpp"
#include "ltb/vlk/objs/vulkan_presentation.hpp"
#include "ltb/vlk/pipeline_layout.hpp"
#include "ltb/vlk/shader_module.hpp"

namespace ltb::vlk::objs
{

struct VulkanGraphicsPipelineSettings
{
    std::vector< ShaderModuleSettings > shader_modules = { };

    uint32 descriptor_set_count = 0U;

    std::vector< std::vector< vk::DescriptorSetLayoutBinding > > uniform_binding_sets = { };

    std::vector< vk::PushConstantRange > uniform_push_constants = { };

    GraphicsPipelineSettings pipeline = { };
};

class VulkanGraphicsPipeline
{
public:
    VulkanGraphicsPipeline( VulkanGpu& gpu, VulkanPresentation& presentation );

    auto initialize( VulkanGraphicsPipelineSettings settings ) -> utils::Result< void >;

    [[nodiscard( "Const getter" )]]
    auto is_initialized( ) const -> bool;

    auto bind( vk::CommandBuffer const& command_buffer ) -> void;

    auto bind_descriptor_sets( FrameInfo const& frame ) -> utils::Result< void >;

    [[nodiscard( "Const getter" )]]
    auto shader_modules( ) const -> std::vector< ShaderModule > const&;
    auto shader_modules( ) -> std::vector< ShaderModule >&;

    [[nodiscard( "Const getter" )]]
    auto descriptor_set_layouts( ) const -> std::vector< DescriptorSetLayout > const&;
    auto descriptor_set_layouts( ) -> std::vector< DescriptorSetLayout >&;

    [[nodiscard( "Const getter" )]]
    auto descriptor_sets( ) const -> std::vector< DescriptorSets > const&;
    auto descriptor_sets( ) -> std::vector< DescriptorSets >&;

    [[nodiscard( "Const getter" )]]
    auto pipeline_layout( ) const -> PipelineLayout const&;
    auto pipeline_layout( ) -> PipelineLayout&;

    [[nodiscard( "Const getter" )]]
    auto pipeline( ) const -> GraphicsPipeline const&;
    auto pipeline( ) -> GraphicsPipeline&;

private:
    VulkanGpu&          gpu_;
    VulkanPresentation& presentation_;

    std::vector< ShaderModule > shader_modules_ = { };

    std::vector< DescriptorSetLayout > descriptor_set_layouts_ = { };
    std::vector< DescriptorSets >      descriptor_sets_        = { };

    PipelineLayout pipeline_layout_ = { gpu_.device( ) };

    GraphicsPipeline pipeline_ = {
        gpu_.device( ),
        presentation_.render_pass( ),
        shader_modules_,
        pipeline_layout_,
    };

    bool initialized_ = false;
};

auto default_initialization(
    VulkanGraphicsPipeline&        setup,
    VulkanGraphicsPipelineSettings settings
) -> utils::Result< void >;

} // namespace ltb::vlk::objs
