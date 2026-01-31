#version 450

layout(push_constant) uniform DisplayUniforms
{
    vec4 color;
} display;

layout(location = 0) out vec4 out_color;

void main()
{
    out_color = display.color;
}
