#version 450

// Draws one producer's latest frame.

layout(set = 0, binding = 0) uniform sampler2D producer_frame;

layout(location = 0) in vec2 uv;
layout(location = 0) out vec4 out_color;

void main() {
    out_color = texture(producer_frame, uv);
}
