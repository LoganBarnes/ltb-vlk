// ///////////////////////////////////////////////////////////////////////////////////////
// A Logan Thomas Barnes project
// ///////////////////////////////////////////////////////////////////////////////////////
#pragma once

// project
// #include "ltb/exec/update_loop.hpp"
// #include "ltb/utils/timers.hpp"
// #include "ltb/vlk/buffer.hpp"
// #include "ltb/vlk/command_buffer.hpp"
// #include "ltb/vlk/command_pool.hpp"
// #include "ltb/vlk/descriptor_pool.hpp"
// #include "ltb/vlk/descriptor_set_layout.hpp"
// #include "ltb/vlk/descriptor_sets.hpp"
// #include "ltb/vlk/device.hpp"
// #include "ltb/vlk/device_memory.hpp"
// #include "ltb/vlk/fence.hpp"
// #include "ltb/vlk/framebuffer.hpp"
// #include "ltb/vlk/graphics_pipeline.hpp"
// #include "ltb/vlk/image.hpp"
// #include "ltb/vlk/image_view.hpp"
// #include "ltb/vlk/instance.hpp"
// #include "ltb/vlk/physical_device.hpp"
// #include "ltb/vlk/pipeline_layout.hpp"
// #include "ltb/vlk/render_pass.hpp"
// #include "ltb/vlk/semaphore.hpp"
// #include "ltb/vlk/shader_module.hpp"
// #include "ltb/vlk/surface.hpp"
// #include "ltb/vlk/swapchain.hpp"
// #include "ltb/window/fwd.hpp"

// project
#include "ltb/utils/result.hpp"

// external
#include <GL/gl3w.h>
#include <GLFW/glfw3.h>

namespace ltb
{

class RawApp

{
public:
    explicit RawApp( );

    auto initialize( ) -> utils::Result< void >;

    [[nodiscard( "Const getter" )]]
    auto running( ) const -> bool;

    auto render( ) const -> void;

    auto cleanup( ) -> void;

private:
    int         glfw_init_ = 0;
    GLFWwindow* window_    = nullptr;
};

} // namespace ltb
