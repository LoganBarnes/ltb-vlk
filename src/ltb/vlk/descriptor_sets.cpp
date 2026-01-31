// ///////////////////////////////////////////////////////////////////////////////////////
// A Logan Thomas Barnes project
// ///////////////////////////////////////////////////////////////////////////////////////
#include "ltb/vlk/descriptor_sets.hpp"

// project
#include "ltb/utils/container_utils.hpp"
#include "ltb/vlk/check.hpp"
#include "ltb/vlk/descriptor_pool.hpp"
#include "ltb/vlk/device.hpp"

// external
#include <range/v3/range/conversion.hpp>
#include <range/v3/view/transform.hpp>

namespace ltb::vlk
{

DescriptorSets::DescriptorSets( Device& device, DescriptorPool& descriptor_pool )
    : device_( device )
    , descriptor_pool_( descriptor_pool )
{
}

auto DescriptorSets::initialize( DescriptorSetsSettings settings ) -> utils::Result< void >
{
    if ( this->is_initialized( ) )
    {
        return utils::success( );
    }
    LTB_CHECK_VALID( device_.is_initialized( ) );
    LTB_CHECK_VALID( descriptor_pool_.is_initialized( ) );

    auto const descriptor_set_info = vk::DescriptorSetAllocateInfo{ }
                                         .setDescriptorPool( descriptor_pool_.get( ) )
                                         .setSetLayouts( settings.layouts );

    VK_CHECK( auto descriptor_sets, device_.get( ).allocateDescriptorSets( descriptor_set_info ) );

    settings_        = std::move( settings );
    descriptor_sets_ = std::move( descriptor_sets );

    return utils::success( );
}

auto DescriptorSets::is_initialized( ) const -> bool
{
    return !descriptor_sets_.empty( );
}

auto DescriptorSets::get( ) const -> std::vector< vk::DescriptorSet > const&
{
    return descriptor_sets_;
}

auto DescriptorSets::get( ) -> std::vector< vk::DescriptorSet >&
{
    return descriptor_sets_;
}

auto DescriptorSets::reset( ) -> void
{
    descriptor_sets_.clear( );
    settings_ = { };
}

auto DescriptorSets::settings( ) const -> DescriptorSetsSettings const&
{
    return settings_;
}

} // namespace ltb::vlk
