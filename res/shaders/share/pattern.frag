#version 450

// An animated pattern a producer renders. `hue` gives each producer its own colors.

layout(push_constant) uniform PatternParams {
    float time;
    float hue;
} params;

layout(location = 0) in vec2 uv;
layout(location = 0) out vec4 out_color;

vec3 hsv_to_rgb(vec3 c) {
    vec3 p = abs(fract(c.xxx + vec3(1.0, 2.0 / 3.0, 1.0 / 3.0)) * 6.0 - 3.0);
    return c.z * mix(vec3(1.0), clamp(p - 1.0, 0.0, 1.0), c.y);
}

void main() {
    vec2  p     = uv * 2.0 - 1.0;
    float t     = params.time;
    float r     = length(p);
    float angle = atan(p.y, p.x);

    vec3 base   = hsv_to_rgb(vec3(params.hue, 0.65, 0.9));
    vec3 accent = hsv_to_rgb(vec3(fract(params.hue + 0.5), 0.55, 1.0));

    // Sharp-edged (but anti-aliased) bands read better than soft gradients when scaled down.
    float rings  = smoothstep(0.42, 0.58, 0.5 + 0.5 * sin(12.0 * r - 3.0 * t));
    float spokes = smoothstep(0.42, 0.58, 0.5 + 0.5 * sin(6.0 * angle + 1.5 * t));

    vec3 color = mix(base * (0.4 + 0.6 * rings), accent, 0.35 * spokes) * (1.0 - 0.35 * r);

    // An orbiting dot makes it obvious the frames are live.
    // Offset by hue so producers started together don't move in lockstep.
    float dot_angle  = t + 6.2831853 * params.hue;
    vec2  dot_center = 0.6 * vec2(cos(dot_angle), sin(dot_angle));
    float dot_mask   = 1.0 - smoothstep(0.10, 0.12, length(p - dot_center));
    color            = mix(color, vec3(1.0), dot_mask);

    out_color = vec4(color, 1.0);
}
