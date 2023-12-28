#pragma once

// Note: This has to be there, linker issues otherwise
#include "include/defines.h"

#include <vulkan/vulkan_core.h>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <glm/mat4x4.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "include/assets.h"


void init_vulkan(GLFWwindow* window);
void init_materials();

void draw_object(glm::mat4* transform, glm::mat4* prev_mvp, Model* model, u32 material);
void draw_rigged(glm::mat4* transform, 
                 glm::mat4* prev_mvp, 
                 Model* model, 
                 u32 material, 
                 Bone* pose, 
                 u32 bone_count);

void end_frame(GLFWwindow* window);
void start_frame(glm::vec3 camera_pos, glm::mat4 proj_view);

void cleanup_vulkan();
