#version 450

// A quad covering `offset - scale` to `offset + scale` in normalized device coordinates.

layout(push_constant) uniform TileParams {
    vec2 offset;
    vec2 scale;
} params;

layout(location = 0) out vec2 uv;

const vec2 corners[6] = vec2[](
    vec2(-1.0, -1.0), vec2(1.0, -1.0), vec2(1.0, 1.0),
    vec2(-1.0, -1.0), vec2(1.0, 1.0), vec2(-1.0, 1.0)
);

void main() {
    vec2 corner = corners[gl_VertexIndex];
    uv          = corner * 0.5 + 0.5;
    gl_Position = vec4(params.offset + corner * params.scale, 0.0, 1.0);
}
