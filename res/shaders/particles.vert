#version 450

layout(location = 0) in vec2 in_position;
layout(location = 1) in vec4 in_color;

layout(binding = 0) uniform CameraBufferObject
{
    mat4 clip_from_world;
} camera;

layout(location = 0) out vec4 frag_color;

void main()
{
    gl_Position = camera.clip_from_world * vec4(in_position, 0.0, 1.0);
    frag_color  = in_color;

    gl_PointSize = 5.0F;
}
