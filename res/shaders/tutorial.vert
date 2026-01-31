#version 450

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec3 in_color;

layout(binding = 0) uniform CameraBufferObject {
    mat4 view;
    mat4 proj;
    mat4 proj_view;
} camera;

layout(push_constant) uniform ModelBufferObject {
    mat4 transform;
} model;

layout(location = 0) out vec3 frag_color;

void main()
{
    gl_Position = camera.proj_view * model.transform * vec4(in_position, 1.0);
    frag_color  = in_color;
}
