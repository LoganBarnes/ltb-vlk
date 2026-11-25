// ///////////////////////////////////////////////////////////////////////////////////////
// A Logan Thomas Barnes project
// ///////////////////////////////////////////////////////////////////////////////////////
#include "ltb/vlk/descriptor_sets.hpp"

// project
#include "ltb/vlk/descriptor_pool.hpp"
#include "ltb/vlk/descriptor_set_layout.hpp"
#include "ltb/vlk/headless_gpu_fixture.hpp"

namespace ltb::vlk
{
namespace
{

using DescriptorSetsTest = HeadlessGpuTest;

TEST_F( DescriptorSetsTest, ResetReturnsSetsToThePool )
{
    // A pool that only has room for one set at a time.
    auto pool = DescriptorPool{ gpu_.device( ) };
    ASSERT_TRUE( pool.initialize( {
                                      .pool_sizes = { { vk::DescriptorType::eUniformBuffer, 1U } },
                                      .max_sets   = 1U,
                                  } )
                     .has_value( ) );

    auto layout = DescriptorSetLayout{ gpu_.device( ) };
    ASSERT_TRUE( layout
                     .initialize( {
                         .bindings = { vk::DescriptorSetLayoutBinding{ }
                                           .setBinding( 0U )
                                           .setDescriptorType( vk::DescriptorType::eUniformBuffer )
                                           .setDescriptorCount( 1U )
                                           .setStageFlags( vk::ShaderStageFlagBits::eVertex ) },
                     } )
                     .has_value( ) );

    auto sets = DescriptorSets{ gpu_.device( ), pool };

    // Without freeing on reset, the second allocation runs the pool out of memory.
    for ( auto i = 0; i < 3; ++i )
    {
        auto const result = sets.initialize( { .layouts = { layout.get( ) } } );
        ASSERT_TRUE( result.has_value( ) )
            << "iteration " << i << ": " << result.error( ).error_message( );
        sets.reset( );
    }
}

} // namespace
} // namespace ltb::vlk
