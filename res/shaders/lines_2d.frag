#version 450

layout(push_constant) uniform DisplayUniforms
{
    layout(offset = 48) vec4 color;
} display;

layout(location = 0) out vec4 out_color;

void main()
{
    out_color = display.color;
}
