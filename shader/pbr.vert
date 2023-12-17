#version 450

layout(binding = 0, set = 0) uniform GlobalUniform 
{
    mat4 proj_view;
    vec3 camera_pos;
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
layout(location = 2) out vec2 out_prev_screen_pos;
layout(location = 3) out vec2 out_screen_pos;

void main() 
{
    vec4 world_pos = object.model * vec4(in_position, 1.0);
    out_normal = (object.model * vec4(in_normal, 0.0)).xyz;
    out_pos = world_pos.xyz;
    gl_Position = global.proj_view * world_pos;

    // taa stuff...
    vec4 prev_pos = object.prev_mvp * vec4(in_position, 1.0);
    out_prev_screen_pos = ((vec2(prev_pos.x, prev_pos.y) / prev_pos.w) + 1) / 2;
    out_screen_pos = ((vec2(gl_Position.x, gl_Position.y) / gl_Position.w) + 1) / 2;
}
