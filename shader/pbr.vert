#version 450

layout(binding = 0, set = 0) uniform GlobalUniform 
{
    mat4 proj;
    mat4 view;
    vec3 camera_pos;
    int align;              // this has to be there for byte alignment
    int jitter_index;
} global;

layout(binding = 0, set = 1) uniform MaterialUniform 
{
    float roughness;
} material;

layout(binding = 0, set = 2) uniform ObjectUniform 
{
    mat4 model;
    mat4 prev_mvp;
} object;

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec3 in_normal;

layout(location = 0) out vec3 out_normal;
layout(location = 1) out vec3 out_pos;
layout(location = 2) out vec3 out_prev_screen_pos;

vec2 jitter_offsets[] = {
    vec2(1, -1),
    vec2(0, 0),
    vec2(-1, 1),
    vec2(1, 1),
    vec2(-1, -1),
};

void main() 
{
    vec4 world_pos = object.model * vec4(in_position, 1.0);
    out_normal = (object.model * vec4(in_normal, 0.0)).xyz;
    out_pos = world_pos.xyz;

    // taa stuff...
    vec4 prev_pos = object.prev_mvp * vec4(in_position, 1.0);
    out_prev_screen_pos = prev_pos.xyw;

    // jittering
    mat4 proj_copy = global.proj;
    vec2 jitter_offset = jitter_offsets[global.jitter_index];
    proj_copy[3][0] += jitter_offset.x / 1280;
    proj_copy[3][1] += jitter_offset.y / 720;
    gl_Position = proj_copy * global.view * world_pos;
}
