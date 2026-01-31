
// Constants
const uint color_mode_white        = 0;
const uint color_mode_position     = 1;
const uint color_mode_normal       = 2;
const uint color_mode_uv_coord     = 3;
const uint color_mode_vertex_color = 4;
const uint color_mode_global_color = 5;
const uint color_mode_texture      = 6;/// \todo

const uint shading_mode_flat          = 0;
const uint shading_mode_lambertian    = 1;/// \todo (need lights)
const uint shading_mode_cook_torrance = 2;/// \todo

// Uniforms
//layout(binding = 1) uniform sampler2D tex_sampler;

layout(push_constant) uniform Display {
    layout(offset = 112) vec4 color; // offset must be the size of previous push_constants
    uint color_mode;
    uint shading_mode;
} display;
