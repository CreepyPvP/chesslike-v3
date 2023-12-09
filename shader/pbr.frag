#version 450

layout(location = 0) in vec3 in_normal;
layout(location = 1) in vec3 in_pos;

layout(location = 0) out vec4 out_color;

int light_count = 1;
vec3 light_colors[] = {
    vec3(0.5, 0.7, 0.4),
    vec3(0.7, 0.5, 0.5)
};
vec3 light_dir[] = {
    normalize(vec3(1, 0, 0)),
    normalize(vec3(-1, 7, 2))
};

void main() 
{
    vec3 camera_pos = vec3(2, 2, 2);
    vec3 n = normalize(in_normal);
    vec3 v = normalize(camera_pos - in_pos);

    out_color = vec4(0, 0, 1, 1);
}
