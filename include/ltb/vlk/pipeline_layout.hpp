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

struct PipelineLayoutSettings
{
    std::vector< vk::DescriptorSetLayout > descriptor_set_layouts = { };
    std::vector< vk::PushConstantRange >   push_constant_ranges   = { };
};

class PipelineLayout
{
public:
    explicit( false ) PipelineLayout( Device& device );

    auto initialize( PipelineLayoutSettings settings ) -> utils::Result< void >;

    [[nodiscard( "Const getter" )]]
    auto is_initialized( ) const -> bool;

    [[nodiscard( "Const getter" )]]
    auto get( ) const -> vk::PipelineLayout const&;
    auto get( ) -> vk::PipelineLayout&;

private:
    Device& device_;

    PipelineLayoutSettings   settings_        = { };
    vk::UniquePipelineLayout pipeline_layout_ = { };
};

template < typename PushConstantType >
auto append_push_constant_range(
    std::vector< vk::PushConstantRange >& push_constant_ranges,
    vk::ShaderStageFlags const            stage_flags
) -> void
{
    auto offset = 0U;
    if ( push_constant_ranges.empty( ) )
    {
        offset = push_constant_ranges.back( ).size;
    }

    push_constant_ranges.push_back(
        vk::PushConstantRange{ }
            .setStageFlags( stage_flags )
            .setOffset( offset )
            .setSize( sizeof( PushConstantType ) )
    );
}

} // namespace ltb::vlk
