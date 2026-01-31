#version 450

layout(location = 0) in vec2 in_position;

layout(binding = 0) uniform CameraUniforms
{
    mat4 clip_from_world;
} camera;

void main()
{
    gl_Position = camera.clip_from_world * vec4(in_position, 0.0F, 1.0F);
}
