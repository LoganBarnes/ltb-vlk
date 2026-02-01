// ///////////////////////////////////////////////////////////////////////////////////////
// A Logan Thomas Barnes project
// ///////////////////////////////////////////////////////////////////////////////////////
#pragma once

// project
#include "ltb/utils/result.hpp"
#include "ltb/vlk/fwd.hpp"
#include "ltb/vlk/vulkan.hpp"

namespace ltb::vlk
{

class ComputePipeline
{
public:
    ComputePipeline( Device& device, ShaderModule& shader_module, PipelineLayout& pipeline_layout );

    auto initialize( ) -> utils::Result< void >;

    [[nodiscard( "Const getter" )]]
    auto is_initialized( ) const -> bool;

    [[nodiscard( "Const getter" )]]
    auto get( ) const -> vk::Pipeline const&;
    auto get( ) -> vk::Pipeline&;

private:
    Device&         device_;
    ShaderModule&   shader_module_;
    PipelineLayout& pipeline_layout_;

    vk::UniquePipeline pipeline_ = { };
};

} // namespace ltb::vlk
