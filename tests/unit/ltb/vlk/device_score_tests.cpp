// ///////////////////////////////////////////////////////////////////////////////////////
// A Logan Thomas Barnes project
// ///////////////////////////////////////////////////////////////////////////////////////
#include "ltb/vlk/detail/device_functors.hpp"

// external
#include <gtest/gtest.h>

namespace ltb::vlk::detail
{
namespace
{

auto make_properties( vk::PhysicalDeviceType const type, uint32 const max_image_dimension )
    -> vk::PhysicalDeviceProperties
{
    auto properties                       = vk::PhysicalDeviceProperties{ };
    properties.deviceType                 = type;
    properties.limits.maxImageDimension2D = max_image_dimension;
    return properties;
}

TEST( DeviceScoreTests, DiscreteBeatsIntegratedRegardlessOfTextureSize )
{
    auto const discrete   = make_properties( vk::PhysicalDeviceType::eDiscreteGpu, 8'192U );
    auto const integrated = make_properties( vk::PhysicalDeviceType::eIntegratedGpu, 32'768U );

    EXPECT_GT( score_physical_device( discrete, 0U ), score_physical_device( integrated, 5U ) );
}

TEST( DeviceScoreTests, DeviceTypesAreRanked )
{
    using enum vk::PhysicalDeviceType;
    auto const score = []( vk::PhysicalDeviceType const type ) -> uint64 {
        return score_physical_device( make_properties( type, 16'384U ), 1U );
    };

    EXPECT_GT( score( eDiscreteGpu ), score( eIntegratedGpu ) );
    EXPECT_GT( score( eIntegratedGpu ), score( eVirtualGpu ) );
    EXPECT_GT( score( eVirtualGpu ), score( eCpu ) );
    EXPECT_GT( score( eCpu ), score( eOther ) );
}

TEST( DeviceScoreTests, ExtensionsBreakTiesBeforeTextureSize )
{
    auto const small = make_properties( vk::PhysicalDeviceType::eDiscreteGpu, 8'192U );
    auto const large = make_properties( vk::PhysicalDeviceType::eDiscreteGpu, 32'768U );

    EXPECT_GT( score_physical_device( small, 2U ), score_physical_device( large, 1U ) );
    EXPECT_GT( score_physical_device( large, 1U ), score_physical_device( small, 1U ) );
}

} // namespace
} // namespace ltb::vlk::detail
