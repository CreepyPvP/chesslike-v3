#version 450

#define PI 3.14
#define NDF_GGX

layout(binding = 0, set = 0) uniform GlobalUniform 
{
    mat4 proj;
    mat4 view;
    vec3 camera_pos;
    int jitter_index;
} global;

layout(binding = 1, set = 0) uniform sampler2D prev_frame;

layout(binding = 0, set = 1) uniform MaterialUniform 
{
    float roughness;
    float smoothness;
    vec3 specular;
    vec3 diffuse;
} material;

layout(location = 0) in vec3 in_normal;
layout(location = 1) in vec3 in_pos;
layout(location = 2) in vec3 in_prev_screen_pos;

layout(location = 0) out vec4 out_color;

int light_count = 2;
vec3 light_colors[] = {
    vec3(1.0, 1.0, 1.0),
    vec3(0.7, 0.2, 0.2)
};

vec3 light_dir[] = {
    normalize(vec3(1, 1, 2)),
    normalize(vec3(-1, 0, 1))
};


float a(vec3 n, vec3 s)
{
    return dot(n, s) / (material.smoothness * sqrt(1 - dot(n, s) * dot(n, s)));
}

float bin_clamp(float x)
{
    return x > 0 ? 1 : 0;
}

#ifdef NDF_GGX
float ndf_lambda(float a)
{
    return (-1 + sqrt(1 + 1 / (a * a))) / 2;
}

float ndf(vec3 n, vec3 m)
{
    float a = material.roughness * material.roughness;
    float x = bin_clamp(dot(n, m)) * a * a;
    float y = 1 + dot(n, m) * dot(n, m) * (a * a - 1);
    float z = PI * y * y;
    return x / z;
}
#endif

vec3 fresnel(vec3 n, vec3 l)
{
    return material.specular + (vec3(1, 1, 1) - material.specular) * pow(1 - clamp(dot(n, l), 0, 1), 5);
}

float lambda(float phi) 
{
    return 1 - exp(-7.3 * phi * phi);
}

float shadow_masking(vec3 l, vec3 v, vec3 m, vec3 n) 
{
    float x = bin_clamp(dot(m, v)) * bin_clamp(dot(m, l));
    float a_v = a(n, v);
    float a_l = a(n, l);
    float y = 1 + ndf_lambda(a_v) + ndf_lambda(a_l);
    return x / y;
}

vec3 brdf(vec3 l, vec3 v, vec3 n) 
{
    vec3 h = normalize(l + v);

    vec3 fresnel_term = fresnel(h, l);
    float divider = 4 * abs(dot(n, l)) * abs(dot(n, v));
    vec3 specular = fresnel_term * ndf(n, h) * shadow_masking(l, v, h, n) / divider;

    vec3 f_smooth = 21 / 20 * (1 - material.specular) * (1 - pow(1 - dot(n, l), 5)) * (1 - pow(1 - dot(n, v), 5));
    float k_facing = 0.5 + 0.5 * dot(l, v);
    float f_rough = k_facing * (0.9 - 0.4 * k_facing) * ((0.5 + dot(n, h)) / (dot(n, h)));
    float f_multi = 0.3641 * material.roughness;
    vec3 smooth_term = material.diffuse * f_multi + mix(f_smooth, f_rough * vec3(1, 1, 1), material.roughness);
    vec3 diffuse = bin_clamp(dot(n, l)) * bin_clamp(dot(n, v)) * material.diffuse / PI * smooth_term;

    return diffuse + specular;
}

void main() 
{
    vec3 n = normalize(in_normal);
    vec3 v = normalize(global.camera_pos - in_pos);

    out_color = vec4(0, 0, 0, 1);
    for (int i = 0; i < light_count; ++i) {
        vec3 l = light_dir[i];
        vec3 c_light = light_colors[i];

        out_color.rgb += brdf(l, v, n) * c_light * clamp(dot(n, l), 0, 1);
    }
    out_color.rgb *= PI;
    // QUESTION: ambient lighting?

    vec2 prev_pos = ((in_prev_screen_pos.xy / in_prev_screen_pos.z) + 1) / 2;
    out_color = 0.50 * out_color + 0.50 * texture(prev_frame, prev_pos);
}
