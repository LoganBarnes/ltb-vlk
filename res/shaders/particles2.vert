#version 450

layout(location = 0) in vec4 in_position;
layout(location = 1) in vec4 in_velocity;

layout(binding = 0) uniform CameraBufferObject
{
    mat4 clip_from_world;
} camera;

layout(location = 0) out vec4 frag_color;

void main()
{
    frag_color = vec4(abs(in_velocity.xyz) / 10.0F, 1.0F);

    gl_Position = camera.clip_from_world * vec4(in_position.xyz, 1.0F);
    gl_PointSize = 5.0F;
}
