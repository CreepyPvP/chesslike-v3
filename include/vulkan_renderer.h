#pragma once

#include <vulkan/vulkan_core.h>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "include/scene.h"


void init_vulkan(GLFWwindow* window);
void init_materials();

void draw_frame(GLFWwindow* window, Scene* scene);

void cleanup_vulkan();
