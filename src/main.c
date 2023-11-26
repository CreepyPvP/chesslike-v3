#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <stdio.h>

#include "defines.h"

typedef struct {
    int width;
    int height;
    GLFWwindow* handle;
} Window;

static Window global_window;

static void resize_cb(GLFWwindow *window, int width, int height)
{
    global_window.width = width;
    global_window.height = height;
    glViewport(0, 0, width, height);
}

static int init_window() 
{
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_SAMPLES, 4);

#ifndef DEBUG
    global_window.width = 1920;
    global_window.height = 1080;
    GLFWmonitor* monitor = glfwGetPrimaryMonitor();
#else
    global_window.width = 960;
    global_window.height = 540;
    GLFWmonitor* monitor = NULL;
#endif
    global_window.handle = glfwCreateWindow(global_window.width,
                                            global_window.height,
                                            "Strategygame",
                                            monitor,
                                            NULL);
    if (!global_window.handle) {
        printf("Failed to create window\n");
        return 1;
    }

    glfwSetFramebufferSizeCallback(global_window.handle, 
                                   resize_cb);
    glfwMakeContextCurrent(global_window.handle);
    return 0;
}

int main()
{
    if (init_window()) {
        printf("Failed to open window\n");
        return 1;
    }
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        printf("failed to load required extensions\n");
        return 3;
    }

    while (!glfwWindowShouldClose(global_window.handle)) {
        if (glfwGetKey(global_window.handle, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
            glfwSetWindowShouldClose(global_window.handle, 1);
        }

        glfwSwapBuffers(global_window.handle);
        glfwPollEvents();
    }

    glfwTerminate();
}
