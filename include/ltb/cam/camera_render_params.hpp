// ///////////////////////////////////////////////////////////////////////////////////////
// A Logan Thomas Barnes project
// ///////////////////////////////////////////////////////////////////////////////////////
#pragma once

// external
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace ltb::cam
{

struct CameraRenderParams
{
    glm::mat4 view_from_world = glm::identity< glm::mat4 >( );
    glm::mat4 clip_from_view  = glm::identity< glm::mat4 >( );
    glm::mat4 clip_from_world = glm::identity< glm::mat4 >( );
};

struct SimpleCameraRenderParams
{
    glm::mat4 clip_from_world = glm::identity< glm::mat4 >( );
};

} // namespace ltb::cam
