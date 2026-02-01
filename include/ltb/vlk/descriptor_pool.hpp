// ///////////////////////////////////////////////////////////////////////////////////////
// A Logan Thomas Barnes project
// ///////////////////////////////////////////////////////////////////////////////////////
#pragma once

// project
#include "ltb/utils/result.hpp"
#include "ltb/utils/types.hpp"
#include "ltb/vlk/fwd.hpp"
#include "ltb/vlk/vulkan.hpp"

namespace ltb::vlk
{

struct DescriptorPoolSettings
{
    constexpr static auto default_descriptor_size = 1000u;

    std::vector< vk::DescriptorPoolSize > pool_sizes = {
        { vk::DescriptorType::eSampler, default_descriptor_size },
        { vk::DescriptorType::eCombinedImageSampler, default_descriptor_size },
        { vk::DescriptorType::eSampledImage, default_descriptor_size },
        { vk::DescriptorType::eStorageImage, default_descriptor_size },
        { vk::DescriptorType::eUniformTexelBuffer, default_descriptor_size },
        { vk::DescriptorType::eStorageTexelBuffer, default_descriptor_size },
        { vk::DescriptorType::eUniformBuffer, default_descriptor_size },
        { vk::DescriptorType::eStorageBuffer, default_descriptor_size },
        { vk::DescriptorType::eUniformBufferDynamic, default_descriptor_size },
        { vk::DescriptorType::eStorageBufferDynamic, default_descriptor_size },
        { vk::DescriptorType::eInputAttachment, default_descriptor_size },
    };

    uint32_t max_sets = default_descriptor_size * static_cast< uint32 >( pool_sizes.size( ) );

    vk::DescriptorPoolCreateFlags flags = vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet;
};

class DescriptorPool
{
public:
    explicit( false ) DescriptorPool( Device& device );

    auto initialize( DescriptorPoolSettings settings ) -> utils::Result< void >;

    [[nodiscard( "Const getter" )]]
    auto is_initialized( ) const -> bool;

    [[nodiscard( "Const getter" )]]
    auto get( ) const -> vk::DescriptorPool const&;
    auto get( ) -> vk::DescriptorPool&;

    [[nodiscard( "Const getter" )]]
    auto settings( ) const -> DescriptorPoolSettings const&;

private:
    Device& device_;

    DescriptorPoolSettings   settings_        = { };
    vk::UniqueDescriptorPool descriptor_pool_ = { };
};

} // namespace ltb::vlk
