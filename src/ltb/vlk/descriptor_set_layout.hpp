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

struct DescriptorSetLayoutSettings
{
    std::vector< vk::DescriptorSetLayoutBinding > bindings = { };
};

class DescriptorSetLayout
{
public:
    explicit( false ) DescriptorSetLayout( Device& device );

    auto initialize( DescriptorSetLayoutSettings settings ) -> utils::Result< void >;

    [[nodiscard( "Const getter" )]]
    auto is_initialized( ) const -> bool;

    [[nodiscard( "Const getter" )]]
    auto get( ) const -> vk::DescriptorSetLayout const&;
    auto get( ) -> vk::DescriptorSetLayout&;

private:
    Device& device_;

    DescriptorSetLayoutSettings   settings_              = { };
    vk::UniqueDescriptorSetLayout descriptor_set_layout_ = { };
};

} // namespace ltb::vlk
