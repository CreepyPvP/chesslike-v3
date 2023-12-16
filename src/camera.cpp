#include <include/camera.h>

#include <glm/geometric.hpp>
#include <glm/mat4x4.hpp>
#include <glm/gtc/matrix_transform.hpp>

#define SPEED 10.0f

void Camera::init() {
    pos = glm::vec3(0, 40, 0);
    front = glm::vec3(0, -1, 0);
    yaw = 0;
    pitch = 0;
};


void Camera::process_key_input(GLFWwindow* window, float delta) {
    glm::vec3 movement = glm::vec3(0.0f);
    bool moved = false;
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
        movement.x += 1;
        moved = true;
    }
    else if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
        movement.x -= 1;
        moved = true;
    }
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
        movement.y -= 1;
        moved = true;
    }
    else if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
        movement.y += 1;
        moved = true;
    }
    
    if (moved) {
        movement = glm::normalize(movement) * SPEED * delta;
        pos += movement.x * front;
        pos += movement.y * glm::normalize(glm::cross(front, glm::vec3(0, 0, 1)));
    }
}

void Camera::process_mouse_input(float x, float y) {
    yaw += x;
    pitch -= y;

    if (pitch > 89.0) {
        pitch = 89.0;
    } else if (pitch < -89.0) {
        pitch = -89.0;
    }

    glm::vec3 direction;
    direction.x = -cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    direction.z = sin(glm::radians(pitch));
    direction.y = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
    front = glm::normalize(direction);
}

Camera camera;
