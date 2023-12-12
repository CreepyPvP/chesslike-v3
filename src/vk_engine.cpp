#include "include/vk_engine.h"

static void resize_cb(GLFWwindow *window, i32 width, i32 height)
{

};

void VkEngine::init()
{
    initialized = false;
    init_window();
    init_vulkan();
    init_swapchain();
    init_commands();
    init_renderpass();
    init_framebuffers();
    initialized = true;
};

void VkEngine::init_vulkan()
{
    vkb::InstanceBuilder builder;
    vkb::Instance vkb_inst = builder
        .set_app_name("myengine")
		.request_validation_layers(true)
		.require_api_version(1, 1, 0)
		.use_default_debug_messenger()
		.build().value();
	instance = vkb_inst.instance;
	debug_messenger = vkb_inst.debug_messenger;

    if (glfwCreateWindowSurface(instance, 
                                window, 
                                NULL, 
                                &surface) != VK_SUCCESS) {
        printf("Failed to create surface\n");
        exit(1);
    }

    vkb::PhysicalDeviceSelector selector{vkb_inst};
    vkb::PhysicalDevice vkb_physical_device = selector
        .set_minimum_version(1, 1)
        .set_surface(surface)
        .select()
        .value();
    vkb::DeviceBuilder device_builder{vkb_physical_device};
    vkb::Device vkb_device = device_builder.build().value();
    device = vkb_device.device;
    physical_device = vkb_physical_device.physical_device;
    graphics_queue = vkb_device.get_queue(vkb::QueueType::graphics).value();
    graphics_queue_family = vkb_device
        .get_queue_index(vkb::QueueType::graphics)
        .value();
};

void VkEngine::init_window() 
{
    width = 1280;
    height = 720;
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
    window = glfwCreateWindow(width, height, "Vulkan", NULL, NULL);
    glfwSetFramebufferSizeCallback(window, resize_cb);
}

void VkEngine::init_swapchain()
{
    vkb::SwapchainBuilder swapchain_builder{physical_device, device, surface};
    vkb::Swapchain vkb_swapchain = swapchain_builder
        .use_default_format_selection()
        .set_desired_present_mode(VK_PRESENT_MODE_FIFO_KHR)
        .set_desired_extent(width, height)
        .build()
        .value();
    swapchain = vkb_swapchain.swapchain;
    swapchain_images = vkb_swapchain.get_images().value();
    swapchain_image_views = vkb_swapchain.get_image_views().value();
    swapchain_image_format = vkb_swapchain.image_format;
}

void VkEngine::init_commands()
{
    VkCommandPoolCreateInfo command_pool_info = cmd_pool_info(
        graphics_queue_family,
        VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT
    );
    if (vkCreateCommandPool(device, 
                            &command_pool_info, 
                            NULL, 
                            &command_pool) != VK_SUCCESS) {
        printf("Failed to create command pool\n");
        exit(1);
    }
    VkCommandBufferAllocateInfo cmd_alloc_info = cmd_buffer_info(
        command_pool, 
        1, 
        VK_COMMAND_BUFFER_LEVEL_PRIMARY
    );
    if (vkAllocateCommandBuffers(device, 
                                 &cmd_alloc_info, 
                                 &main_command_buffer) != VK_SUCCESS) {
        printf("Failed to allocate command buffer\n");
        exit(1);
    }
}

void VkEngine::init_renderpass()
{
    VkAttachmentDescription color_attachment{};
    color_attachment.format = swapchain_image_format;
    color_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
    color_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    color_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    color_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    color_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    color_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    color_attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    VkAttachmentReference color_attachment_ref{};
    color_attachment_ref.attachment = 0;
    color_attachment_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &color_attachment_ref;
    VkRenderPassCreateInfo render_pass_info{};
    render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    render_pass_info.attachmentCount = 1;
    render_pass_info.pAttachments = &color_attachment;
    render_pass_info.subpassCount = 1;
    render_pass_info.pSubpasses = &subpass;
    if (vkCreateRenderPass(device, 
                           &render_pass_info, 
                           NULL, 
                           &render_pass) != VK_SUCCESS) {
        printf("Failed to create render pass\n");
        exit(1);
    }
}

void VkEngine::init_framebuffers()
{
    VkFramebufferCreateInfo fb_info{};
    fb_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    fb_info.pNext = NULL;
    fb_info.renderPass = render_pass;
    fb_info.width = width;
    fb_info.height = height;
    fb_info.layers = 1;
    fb_info.attachmentCount = 1;
    u32 swapchain_image_count = swapchain_images.size();
    framebuffers = std::vector<VkFramebuffer>(swapchain_image_count);
    for (u32 i = 0; i < swapchain_image_count; ++i) {
        fb_info.pAttachments = &swapchain_image_views[i];
        if (vkCreateFramebuffer(device, 
                                &fb_info, 
                                NULL, 
                                &framebuffers[i]) != VK_SUCCESS) {
            printf("Failed to create framebuffers\n");
            exit(1);
        }
    }
}

void VkEngine::cleanup()
{
    if (!initialized) return;
    vkDestroyRenderPass(device, render_pass, NULL);
    vkDestroyCommandPool(device, command_pool, NULL);
    vkDestroySwapchainKHR(device, swapchain, NULL);
    for (i32 i = 0; i < swapchain_image_views.size(); ++i) {
        vkDestroyFramebuffer(device, framebuffers[i], NULL);
        vkDestroyImageView(device, swapchain_image_views[i], NULL);
    }
    vkDestroyDevice(device, NULL);
    vkDestroySurfaceKHR(instance, surface, NULL);
    vkb::destroy_debug_utils_messenger(instance, debug_messenger);
    vkDestroyInstance(instance, NULL);
    glfwDestroyWindow(window);
    glfwTerminate();
}

VkEngine engine;
