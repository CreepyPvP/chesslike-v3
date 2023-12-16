#pragma once

#include <GLFW/glfw3.h>
#include <include/defines.h>
#include <glm/ext/matrix_float4x4.hpp>
#include <glm/ext/vector_float3.hpp>

struct Camera
{
    glm::vec3 pos;
    glm::vec3 front;
    float yaw;
    float pitch;

    void init();
    void process_key_input(GLFWwindow* window, float delta);
    void process_mouse_input(float x, float y);
};

extern Camera camera;
