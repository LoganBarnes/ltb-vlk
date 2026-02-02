#version 450

layout(location = 0) in vec2 in_position;

layout(binding = 0) uniform CameraUniforms
{
    mat4 clip_from_world;
} camera;

layout(push_constant) uniform ModelUniforms
{
    mat3 world_from_local;
} model;

void main()
{
    vec3 world_position = model.world_from_local * vec3(in_position, 1.0F);
    gl_Position         = camera.clip_from_world * vec4(world_position, 1.0F);
}
