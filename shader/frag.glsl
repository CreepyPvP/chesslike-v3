#version 450
#pragma shader_stage(fragment)

layout(location = 0) in vec3 in_normal;
layout(location = 1) in vec3 in_pos;

layout(location = 0) out vec4 out_color;

int light_count = 2;
vec3 light_colors[] = {
    vec3(0.5, 0.7, 0.4),
    vec3(0.7, 0.5, 0.5)
};
vec3 light_dir[] = {
    normalize(vec3(-1, 7, 2)),
    normalize(vec3(1, 0, 0)),
};

void main() 
{
    vec3 camera_pos = vec3(2, 2, 2);
    vec3 n = normalize(in_normal);
    vec3 v = normalize(camera_pos - in_pos);

    vec3 c_surface = vec3(0.8, 0.8, 0.8);
    vec3 c_cool = vec3(0, 0, 0.55) + 0.25 * c_surface;
    vec3 c_warm = vec3(0.3, 0.3, 0) + 0.25 * c_surface;
    vec3 c_highlight = vec3(2, 2, 2);

    out_color = vec4(0.5 * c_cool, 1.0);
    for (int i = 0; i < light_count; ++i) {
        vec3 l = light_dir[i];
        vec3 c_light = light_colors[i];
        vec3 r = reflect(-l, n);
        float s = clamp(100 * dot(r, v) - 97, 0, 1);
        vec3 c_wh = mix(c_warm, c_highlight, s);
        out_color.rgb += clamp(dot(l, n), 0, 1) * c_light * c_wh;
    }
}
