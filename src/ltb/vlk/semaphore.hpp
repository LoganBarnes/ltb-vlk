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

struct SemaphoreSettings
{
    vk::SemaphoreCreateFlags flags = { };
};

class Semaphore
{
public:
    explicit( false ) Semaphore( Device& device );

    auto initialize( ) -> utils::Result< void >;
    auto initialize( SemaphoreSettings settings ) -> utils::Result< void >;

    [[nodiscard( "Const getter" )]]
    auto is_initialized( ) const -> bool;

    [[nodiscard( "Const getter" )]]
    auto get( ) const -> vk::Semaphore const&;
    auto get( ) -> vk::Semaphore&;

private:
    Device& device_;

    SemaphoreSettings   settings_  = { };
    vk::UniqueSemaphore semaphore_ = { };
};

} // namespace ltb::vlk
