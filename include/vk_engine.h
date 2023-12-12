#include <vulkan/vulkan_core.h>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include "VkBootstrap.h"

#include "include/vk_initializers.h"
#include "include/defines.h"

#include <vector>

struct VkEngine
{
    VkInstance instance;
    VkDebugUtilsMessengerEXT debug_messenger;
    VkPhysicalDevice physical_device;
    VkDevice device;
    VkSurfaceKHR surface;
    bool initialized;

    GLFWwindow* window;
    i32 width;
    i32 height;

    VkSwapchainKHR swapchain;
    VkFormat swapchain_image_format;
    std::vector<VkImage> swapchain_images;
    std::vector<VkImageView> swapchain_image_views;

    VkQueue graphics_queue;
    u32 graphics_queue_family;
    VkCommandPool command_pool;
    VkCommandBuffer main_command_buffer;

    VkRenderPass render_pass;
    std::vector<VkFramebuffer> framebuffers;

    void init();
    void init_vulkan();
    void init_window();
    void init_swapchain();
    void init_commands();
    void init_renderpass();
    void init_framebuffers();
    void cleanup();
};

extern VkEngine engine;
