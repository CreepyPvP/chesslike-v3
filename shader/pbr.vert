#version 450

layout(binding = 0) uniform UniformBufferObject 
{
    mat4 model;
    mat4 proj_view;
} ubo;

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec3 in_normal;

layout(location = 0) out vec3 out_normal;
layout(location = 1) out vec3 out_pos;

void main() 
{
    vec4 world_pos = ubo.model * vec4(in_position, 1.0);
    out_normal = (ubo.model * vec4(in_normal, 0.0)).xyz;
    out_pos = world_pos.xyz;
    gl_Position = ubo.proj_view * world_pos;
}
