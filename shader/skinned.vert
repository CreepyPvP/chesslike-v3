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

layout(binding = 0, set = 3) uniform BoneUniform
{
    mat4 transforms[20];
} bones;

layout(push_constant) uniform Constants
{
    uint bone_offset;
} constants;


layout(location = 0) in vec3 in_position;
layout(location = 1) in vec3 in_normal;
layout(location = 2) in ivec3 in_bone_ids;
layout(location = 3) in vec3 in_bone_weights;

layout(location = 0) out vec3 out_normal;
layout(location = 1) out vec3 out_pos;
layout(location = 2) out vec3 out_prev_screen_pos;

void main() 
{
    mat4 bone_transform;
    bone_transform = in_bone_weights.x * bones.transforms[constants.bone_offset + in_bone_ids.x];
    bone_transform += in_bone_weights.y * bones.transforms[constants.bone_offset + in_bone_ids.y];
    bone_transform += in_bone_weights.z * bones.transforms[constants.bone_offset + in_bone_ids.z];

    vec4 world_pos = object.model * bone_transform * vec4(in_position, 1.0);
    out_normal = (object.model * bone_transform * vec4(in_normal, 0.0)).xyz;
    out_pos = world_pos.xyz;
    gl_Position = global.proj_view * world_pos;

    // taa stuff...
    vec4 prev_pos = object.prev_mvp * vec4(in_position, 1.0);
    out_prev_screen_pos = prev_pos.xyw;
}
