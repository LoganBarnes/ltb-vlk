// Uniforms
layout(binding = 0) uniform Camera {
    mat4 clip_from_world;
} camera;

//layout(push_constant) uniform Model {
//    mat4 world_from_local;
//    mat3 world_from_local_normals;
////    uint id;
//} model;
