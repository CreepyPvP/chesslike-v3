#include "include/defines.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <math.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <vector>
#include <glm/mat4x4.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "include/utils.h"
#include "include/assets.h"
#include "include/arena.h"
#include "include/scene.h"
#include "include/loading.h"
#include "include/camera.h"
#include "include/vulkan_renderer.h"

const u32 width = 1280;
const u32 height = 720;

GLFWwindow *window;
float last_mouse_pos_x;
float last_mouse_pos_y;

bool frame_buffer_resized = false;

glm::mat4 proj;

Scene scene;


void resize_callback(GLFWwindow *window, i32 width, i32 height) 
{
    frame_buffer_resized = true;
    proj = glm::perspective(glm::radians(45.0f), 
                            (float) width / (float) height, 
                            0.1f, 1000.0f);
    proj[1][1] *= -1;
}

void mouse_callback(GLFWwindow* window, double pos_x, double pos_y) 
{
    float x_offset = pos_x - last_mouse_pos_x;
    float y_offset = pos_y - last_mouse_pos_y;
    last_mouse_pos_x = pos_x;
    last_mouse_pos_y = pos_y;
    const float sensitivity = 0.1f;
    x_offset *= sensitivity;
    y_offset *= sensitivity;
    camera.process_mouse_input(x_offset, y_offset);
}

void init_window() 
{
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
    window = glfwCreateWindow(width, height, "Vulkan", NULL, NULL);
    glfwSetFramebufferSizeCallback(window, resize_callback);

    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    glfwSetCursorPosCallback(window, mouse_callback);
}

void init_allocators()
{
    init_pool(&pool);
    init_arena(vertex_arena, &pool);
    init_arena(vertex_arena + 1, &pool);
    init_arena(index_arena, &pool);
    init_arena(index_arena + 1, &pool);
    init_arena(&asset_arena, &pool);
}

glm::mat4 get_actor_transform(Actor* actor)
{
    glm::mat4 res;
    res = glm::mat4(1.0);
    res = glm::translate(res, glm::vec3(actor->x, actor->y, actor->z));
    res = glm::rotate(res, glm::radians(actor->rot_x), glm::vec3(1.0f, 0.0f, 0.0f));
    res = glm::rotate(res, glm::radians(actor->rot_y), glm::vec3(0.0f, 1.0f, 0.0f));
    res = glm::rotate(res, glm::radians(actor->rot_z), glm::vec3(0.0f, 0.0f, 1.0f));
    res = glm::scale(res, glm::vec3(actor->scale_x, actor->scale_y, actor->scale_z));
    return res;
}

i32 main() 
{
    init_allocators();
    init_window();
    init_scene(&scene);
    camera.init();
    source_file("assets/scene.end", &scene);

    init_vulkan(window);
    init_materials();

    proj = glm::perspective(glm::radians(45.0f), 
                            (float) width / (float) height, 
                            0.1f, 1000.0f);
    proj[1][1] *= -1;

    // current_frame = 0;
    float time_last_frame = glfwGetTime();
    float delta = 0;

    while (!glfwWindowShouldClose(window)) {
        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
            glfwSetWindowShouldClose(window, true);
        }

        float current_time = glfwGetTime();
        delta = current_time - time_last_frame;
        time_last_frame = current_time;

        camera.process_key_input(window, delta);

        glm::mat4 view = glm::lookAt(camera.pos, camera.pos + camera.front, glm::vec3(0.0, 0.0, 1.0));
        glm::mat4 proj_view = proj * view;

        start_frame(camera.pos, proj_view);

        Bone bones[2] = {
            glm::mat4(1.0f),
            glm::rotate(glm::mat4(1.0f), glm::radians(45.0f), glm::vec3(1.0f, 0.0f, 0.0f))
        };

        for (u32 i = 0; i < scene.actor_count; ++i) {
            Actor actor = scene.actors[i];
            glm::mat4 transform = get_actor_transform(&actor);
            if (actor.model->flags & MODEL_FLAG_SKINNED) {
                draw_rigged(&transform, &actor.prev_mvp, actor.model, actor.material, bones, 2);
            } else {
                draw_object(&transform, &actor.prev_mvp, actor.model, actor.material);
            }
            scene.actors[i].prev_mvp = proj_view * transform;
        }

        end_frame(window);

        // current_frame = (current_frame + 1) % max_frames_in_flight;
        glfwPollEvents();
    }
    glfwDestroyWindow(window);
    glfwTerminate();
    cleanup_vulkan();
}
