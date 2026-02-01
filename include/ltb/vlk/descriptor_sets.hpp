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

struct DescriptorSetsSettings
{
    std::vector< vk::DescriptorSetLayout > layouts = { };
};

class DescriptorSets
{
public:
    DescriptorSets( Device& device, DescriptorPool& descriptor_pool );

    auto initialize( DescriptorSetsSettings settings ) -> utils::Result< void >;

    [[nodiscard( "Const getter" )]]
    auto is_initialized( ) const -> bool;

    [[nodiscard( "Const getter" )]]
    auto get( ) const -> std::vector< vk::DescriptorSet > const&;
    auto get( ) -> std::vector< vk::DescriptorSet >&;

    auto reset( ) -> void;

    [[nodiscard( "Const getter" )]]
    auto settings( ) const -> DescriptorSetsSettings const&;

private:
    Device&         device_;
    DescriptorPool& descriptor_pool_;

    DescriptorSetsSettings           settings_        = { };
    std::vector< vk::DescriptorSet > descriptor_sets_ = { };
};

} // namespace ltb::vlk
