#include <vulkan/vulkan_core.h>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <math.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <vector>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEFAULT_ALIGNED_GENTYPES
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/mat4x4.hpp>
#include <glm/gtc/matrix_transform.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include "defines.h"

#define QUEUE_FAMILY_GRAPHICS 1 << 0
#define QUEUE_FAMILY_PRESENT 1 << 1

const u32 width = 1280;
const u32 height = 720;
const i32 max_frames_in_flight = 2;
const char* validation_layers[] = {
    "VK_LAYER_KHRONOS_validation",
};
const i32 validation_layer_count = 1;
const char* device_extensions[] = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME,
};
const i32 device_extension_count = 1;
#ifdef DEBUG
const bool enable_validation_layers = true;
#else
const bool enable_validation_layers = false;
#endif


struct QueueFamilyIndices
{
    u32 flags;
    u32 graphics;
    u32 present;
};

struct SwapChainSupportDetails 
{
    VkSurfaceCapabilitiesKHR capabilities;
    VkSurfaceFormatKHR* formats;
    u32 format_count;
    VkPresentModeKHR* present_modes;
    u32 present_count;
};

struct Vertex 
{
    float x;
    float y;
    float z;
    float r;
    float g;
    float b;
};

struct UniformBufferObject
{
    glm::mat4 model;
    glm::mat4 view;
    glm::mat4 proj;
};

Vertex* vertices;
u32 vertex_count;
u32* indices;
u32 index_count;

GLFWwindow *window;
QueueFamilyIndices queue_indices;
VkInstance instance;
VkSurfaceKHR surface;
VkSwapchainKHR swap_chain;
std::vector<VkImage> swap_chain_images;
std::vector<VkImageView> swap_chain_image_views;
VkImageView* image_views;
u32 image_count;
VkFormat swap_chain_image_format;
VkExtent2D swap_chain_extent;
VkDevice device;
VkPhysicalDevice physical_device;
VkQueue graphics_queue;
VkQueue present_queue;
VkRenderPass render_pass;
VkDescriptorSetLayout descriptor_set_layout;
VkPipelineLayout pipeline_layout;
VkPipeline graphics_pipeline;
std::vector<VkFramebuffer> swap_chain_framebuffers;
VkCommandPool command_pool;
std::vector<VkCommandBuffer> command_buffers;
std::vector<VkSemaphore> image_available_semaphores;
std::vector<VkSemaphore> render_finished_semaphores;
std::vector<VkFence> in_flight_fences;
u8 frame_buffer_resized;
VkBuffer vertex_buffer;
VkDeviceMemory vertex_buffer_memory;
VkBuffer index_buffer;
VkDeviceMemory index_buffer_memory;
std::vector<VkBuffer> uniform_buffers;
std::vector<VkDeviceMemory> uniform_buffers_memory;
std::vector<void*> uniform_buffers_mapped;
VkDescriptorPool descriptor_pool;
std::vector<VkDescriptorSet> descriptor_sets;
u32 current_frame;
VkImage depth_image;
VkDeviceMemory depth_image_memory;
VkImageView depth_image_view;
VkImage texture_image;
VkDeviceMemory texture_image_memory;
VkImageView texture_image_view;
VkSampler texture_sampler;

static void resize_callback(GLFWwindow *window, i32 width, i32 height) 
{
    frame_buffer_resized = true;
}


static bool is_complete(QueueFamilyIndices* self) 
{
    return self->flags & (QUEUE_FAMILY_GRAPHICS | QUEUE_FAMILY_PRESENT);
}

static VkVertexInputBindingDescription bind_desc() 
{
    VkVertexInputBindingDescription desc{};
    desc.binding = 0;
    desc.stride = sizeof(Vertex);
    desc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
    return desc;
}

static void get_attr_desc(VkVertexInputAttributeDescription* attr_desc)
{
    attr_desc[0].binding = 0;
    attr_desc[0].location = 0;
    attr_desc[0].format = VK_FORMAT_R32G32B32_SFLOAT;
    attr_desc[0].offset = offsetof(Vertex, x);
    attr_desc[1].binding = 0;
    attr_desc[1].location = 1;
    attr_desc[1].format = VK_FORMAT_R32G32B32_SFLOAT;
    attr_desc[1].offset = offsetof(Vertex, r);
}

void init_window() 
{
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
    window = glfwCreateWindow(width, height, "Vulkan", NULL, NULL);
    glfwSetFramebufferSizeCallback(window, resize_callback);
}

u8 check_validation_layer_support() 
{
    u32 layer_count;
    vkEnumerateInstanceLayerProperties(&layer_count, NULL);
    VkLayerProperties* available_layers = (VkLayerProperties*) 
        malloc(sizeof(VkLayerProperties) * layer_count);
    vkEnumerateInstanceLayerProperties(&layer_count, available_layers);
    for (u32 i = 0; i < validation_layer_count; ++i) {
        const char* layer_name = validation_layers[i];
        u8 layer_found = 0;
        for (u32 j = 0; j < layer_count; ++j) {
            VkLayerProperties layer_properties = available_layers[j];
            if (strcmp(layer_name, layer_properties.layerName) == 0) {
                layer_found = 1;
                break;
            }
        }
        if (!layer_found) {
            return 0;
        }
    }
    return 1;
}

Err create_instance() 
{
    if (!check_validation_layer_support()) {
        printf("Validation layers requested, but not available\n");
        return 1;
    }
    VkApplicationInfo app_info{};
    app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    app_info.pApplicationName = "Hello Triangle";
    app_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    app_info.pEngineName = "No Engine";
    app_info.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    app_info.apiVersion = VK_API_VERSION_1_0;
    VkInstanceCreateInfo create_info{};
    create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    create_info.pApplicationInfo = &app_info;
    u32 glfw_extension_count = 0;
    const char **glfw_extensions;
    glfw_extensions = glfwGetRequiredInstanceExtensions(&glfw_extension_count);
    create_info.enabledExtensionCount = glfw_extension_count;
    create_info.ppEnabledExtensionNames = glfw_extensions;
    create_info.enabledLayerCount = 0;
    if (enable_validation_layers) {
        printf("Validation layers enabled\n");
        create_info.enabledLayerCount = validation_layer_count;
        create_info.ppEnabledLayerNames = validation_layers;
    } else {
        create_info.enabledLayerCount = 0;
    }
    if (vkCreateInstance(&create_info, NULL, &instance)) {
        printf("Failed to create instance\n");
        return 2;
    }
    return 0;
}

Err create_surface() 
{
    if (glfwCreateWindowSurface(instance, 
                                window, 
                                NULL, 
                                &surface) != VK_SUCCESS) {
        printf("Failed to create window surface\n");
        return 1;
    }
    return 0;
}

u8 check_device_extension_support(VkPhysicalDevice device) 
{
    u32 extension_count;
    vkEnumerateDeviceExtensionProperties(device, NULL, &extension_count, NULL);
    VkExtensionProperties* available_extensions = 
            (VkExtensionProperties*) malloc(
            sizeof(VkExtensionProperties) * extension_count);
    vkEnumerateDeviceExtensionProperties(device, NULL, &extension_count,
                                         available_extensions);
    for (u32 i = 0; i < device_extension_count; ++i) {
        u8 found = 0;
        for (u32 j = 0; j < extension_count; ++j) {
            if (strcmp(device_extensions[i], 
                       available_extensions[j].extensionName) == 0) {
                found = 1;
                break;
            }
        }
        if (!found)
            return 0;
    }
    return 1;
  }

SwapChainSupportDetails query_swap_chain_support(VkPhysicalDevice device) 
{
    SwapChainSupportDetails details{};
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, 
                                              surface,
                                              &details.capabilities);
    u32 format_count;
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, 
                                         surface, 
                                         &format_count,
                                         NULL);
    if (format_count > 0) {
        details.formats = (VkSurfaceFormatKHR*) 
            malloc(sizeof(VkSurfaceFormatKHR) * format_count);
        details.format_count = format_count;
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, 
                                             surface, 
                                             &format_count,
                                             details.formats);
    }
    u32 present_mode_count;
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, 
                                              surface,
                                              &present_mode_count, 
                                              NULL);
    if (format_count > 0) {
        details.present_modes = (VkPresentModeKHR*) 
            malloc(sizeof(VkPresentModeKHR) * present_mode_count);
        details.present_count = present_mode_count;
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, 
                                                  surface, 
                                                  &present_mode_count, 
                                                  details.present_modes);
    }
    return details;
}

QueueFamilyIndices find_queue_families(VkPhysicalDevice device) 
{
    u32 count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &count, NULL);
    VkQueueFamilyProperties* families = (VkQueueFamilyProperties*) 
        malloc(sizeof(VkQueueFamilyProperties) * count);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &count, families);
    QueueFamilyIndices queue_indices;
    queue_indices.flags = 0;
    for (u32 i = 0; i < count; ++i) {
        VkQueueFamilyProperties queue_family = families[i];
        VkBool32 present_support = 0;
        vkGetPhysicalDeviceSurfaceSupportKHR(device, 
                                             i, 
                                             surface,
                                             &present_support);
        if (present_support) {
            queue_indices.present = i;
            queue_indices.flags |= QUEUE_FAMILY_PRESENT;
        }
        if (queue_family.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            queue_indices.graphics = i;
            queue_indices.flags |= QUEUE_FAMILY_GRAPHICS;
        }
        if (is_complete(&queue_indices)) {
            break;
        }
    }
    return queue_indices;
}


bool is_device_suitable(VkPhysicalDevice device) 
{
    // VkPhysicalDeviceProperties device_properties;
    // VkPhysicalDeviceFeatures device_features;
    // vkGetPhysicalDeviceProperties(device, &device_properties);
    // vkGetPhysicalDeviceFeatures(device, &device_features);
    queue_indices = find_queue_families(device);
    bool extensions_support = check_device_extension_support(device);
    if (extensions_support) {
        SwapChainSupportDetails swap_chain_support =
            query_swap_chain_support(device);
        if (swap_chain_support.format_count == 0 ||
            swap_chain_support.present_count == 0) {
            return false;
        }
    }
    return is_complete(&queue_indices) && extensions_support;
}

Err pick_physical_device(VkPhysicalDevice* device) 
{
    u32 device_count = 0;
    vkEnumeratePhysicalDevices(instance, &device_count, NULL);
    if (device_count == 0) {
        printf("Failed to find GPU with Vulkan support!\n");
        return 1;
    }
    VkPhysicalDevice* devices = 
        (VkPhysicalDevice*) malloc(sizeof(VkPhysicalDevice) * device_count);
    vkEnumeratePhysicalDevices(instance, &device_count, devices);
    for (u32 i = 0; i < device_count; ++i) {
        VkPhysicalDevice current = devices[i];
        if (is_device_suitable(current)) {
            *device = current;
            return 0;
        }
    }

    printf("Failed to find a suitable GPU\n");
    return 2;
}

Err create_logical_device() 
{
    VkDeviceQueueCreateInfo queue_create_infos[2];
    u32 queue_families[2];
    queue_families[0] = queue_indices.graphics;
    queue_families[1] = queue_indices.present;
    u32 queue_fam_count = 2;
    if (queue_indices.graphics == queue_indices.present) {
        queue_fam_count = 1;
    }
    float queue_proiority = 1.0f;
    for (u32 i = 0; i < queue_fam_count; ++i) {
        u32 queue_family = queue_families[i];
        VkDeviceQueueCreateInfo queue_create_info{};
        queue_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queue_create_info.queueFamilyIndex = queue_family; 
        queue_create_info.queueCount = 1;
        queue_create_info.pQueuePriorities = &queue_proiority;
        queue_create_infos[i] = queue_create_info;
    }
    VkPhysicalDeviceFeatures device_features{};
    device_features.samplerAnisotropy = VK_TRUE;
    VkDeviceCreateInfo create_info{};
    create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    create_info.queueCreateInfoCount = queue_fam_count;
    create_info.pQueueCreateInfos = queue_create_infos;
    create_info.pEnabledFeatures = &device_features;
    create_info.enabledExtensionCount = device_extension_count;
    create_info.ppEnabledExtensionNames = device_extensions;
    if (enable_validation_layers) {
        create_info.enabledLayerCount = validation_layer_count;
        create_info.ppEnabledLayerNames = validation_layers;
    } else {
        create_info.enabledLayerCount = 0;
        create_info.ppEnabledLayerNames = NULL;
    }
    if (vkCreateDevice(physical_device, &create_info, NULL, &device)) {
        printf("Failed to create logical device\n");
        return 1;
    }
    vkGetDeviceQueue(device, queue_indices.graphics, 0, &graphics_queue);
    vkGetDeviceQueue(device, queue_indices.present, 0, &present_queue);
    return 0;
}
  
VkSurfaceFormatKHR choose_swap_surface_format(VkSurfaceFormatKHR* formats, 
                                              u32 format_count)
{
    for (u32 i = 0; i < format_count; ++i) {
        VkSurfaceFormatKHR format = formats[i];
        if (format.format == VK_FORMAT_B8G8R8A8_SRGB &&
            format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            return format;
        }
    }

    return formats[0];
}

VkPresentModeKHR choose_swap_present_mode(VkPresentModeKHR* present_modes, 
                                          u32 present_mode_count) 
{
    for (u32 i = 0; i < present_mode_count; ++i) {
        VkPresentModeKHR present_mode = present_modes[i];
        if (present_mode == VK_PRESENT_MODE_MAILBOX_KHR) {
            return present_mode;
        }
    }
    return VK_PRESENT_MODE_FIFO_KHR;
}

u32 clamp(u32 v, u32 lo, u32 hi)
{
    if (v >= hi)
        return hi;
    else if (v <= lo)
        return lo;

    return v;
}

VkExtent2D choose_swap_extent(const VkSurfaceCapabilitiesKHR* capabilities) 
{
    if (capabilities->currentExtent.width != UINT_MAX) {
        return capabilities->currentExtent;
    } else {
        i32 width, height;
        glfwGetFramebufferSize(window, &width, &height);
        VkExtent2D actual_extent = {(u32) width, (u32) height};
        actual_extent.width = clamp(actual_extent.width, 
                                    capabilities->minImageExtent.width,
                                    capabilities->maxImageExtent.width);
        actual_extent.height = clamp(actual_extent.height, 
                                     capabilities->minImageExtent.height,
                                    capabilities->maxImageExtent.height);
        return actual_extent;
    }
}

Err create_swap_chain() 
{
    SwapChainSupportDetails swap_chain_support =
        query_swap_chain_support(physical_device);
    VkSurfaceFormatKHR surface_format = choose_swap_surface_format(
                                    swap_chain_support.formats,
                                    swap_chain_support.format_count);
    VkPresentModeKHR present_mode = choose_swap_present_mode(
                                        swap_chain_support.present_modes, 
                                        swap_chain_support.present_count);
    VkExtent2D extent = choose_swap_extent(&swap_chain_support.capabilities);
    u32 image_count = swap_chain_support.capabilities.minImageCount + 1;
    if (swap_chain_support.capabilities.maxImageCount > 0 &&
        image_count > swap_chain_support.capabilities.maxImageCount) {
        image_count = swap_chain_support.capabilities.maxImageCount;
    }
    VkSwapchainCreateInfoKHR create_info{};
    create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    create_info.surface = surface;
    create_info.minImageCount = image_count;
    create_info.imageFormat = surface_format.format;
    create_info.imageColorSpace = surface_format.colorSpace;
    create_info.imageExtent = extent;
    create_info.imageArrayLayers = 1;
    create_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    u32 queue_family_indices[] = {
        queue_indices.graphics,
        queue_indices.present
    };
    if (queue_indices.present != queue_indices.graphics) {
      create_info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
      create_info.queueFamilyIndexCount = 2;
      create_info.pQueueFamilyIndices = queue_family_indices;
    } else {
      create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
      create_info.queueFamilyIndexCount = 0;
      create_info.pQueueFamilyIndices = NULL;
    }
    create_info.preTransform = swap_chain_support.capabilities.currentTransform;
    create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    create_info.presentMode = present_mode;
    create_info.clipped = VK_TRUE;
    create_info.oldSwapchain = VK_NULL_HANDLE;
    if (vkCreateSwapchainKHR(device, &create_info, NULL, &swap_chain) !=
        VK_SUCCESS) {
        printf("Failed to create swap chain!\n");
        return 1;
    }
    vkGetSwapchainImagesKHR(device, swap_chain, &image_count, NULL);
    swap_chain_images.resize(image_count);
    vkGetSwapchainImagesKHR(device, 
                            swap_chain, 
                            &image_count,
                            swap_chain_images.data());
    swap_chain_image_format = surface_format.format;
    swap_chain_extent = extent;
    return 0;
}

VkImageView create_image_view(VkImage image, 
                              VkFormat format, 
                              VkImageAspectFlags aspect_flags) 
{
    VkImageViewCreateInfo view_info{};
    view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    view_info.image = image;
    view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
    view_info.format = format;
    view_info.subresourceRange.aspectMask = aspect_flags;
    view_info.subresourceRange.baseMipLevel = 0;
    view_info.subresourceRange.levelCount = 1;
    view_info.subresourceRange.baseArrayLayer = 0;
    view_info.subresourceRange.layerCount = 1;
    VkImageView image_view;
    if (vkCreateImageView(device, 
                          &view_info, 
                          NULL, 
                          &image_view) != VK_SUCCESS) {
        // TODO: do something about this one
        printf("Failed to create texture image view\n");
    }
    return image_view;
}

Err create_image_views() 
{
    swap_chain_image_views.resize(swap_chain_images.size());
    for (u32 i = 0; i < swap_chain_images.size(); ++i) {
        swap_chain_image_views[i] = create_image_view(swap_chain_images[i], 
                                                    swap_chain_image_format,
                                                    VK_IMAGE_ASPECT_COLOR_BIT);
    }
    return 0;
}

Err create_shader_module(const char* code, u32 len, VkShaderModule* module) 
{
    VkShaderModuleCreateInfo create_info{};
    create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    create_info.codeSize = len;
    create_info.pCode = (u32*) code;
    if (vkCreateShaderModule(device, &create_info, NULL, module) !=
        VK_SUCCESS) {
        printf("Failed to create shader module\n");
        return 1;
    }
    return 0;
}

char* read_file(const char* file, i32* flen)
{
    FILE* fptr = fopen(file, "rb");
    if (fptr == NULL) {
        printf("Failed to read file: %s\n", file);
        return NULL;
    }
    fseek(fptr, 0, SEEK_END);
    i32 len = ftell(fptr);
    char* buf = (char*) malloc(len);
    fseek(fptr, 0, SEEK_SET);
    fread(buf, len, 1, fptr);
    *flen = len;
    fclose(fptr);
    return buf;
}

Err create_descriptor_set_layout() 
{
    VkDescriptorSetLayoutBinding layout_binding{};
    layout_binding.binding = 0;
    layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    layout_binding.descriptorCount = 1;
    layout_binding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    layout_binding.pImmutableSamplers = NULL;
    VkDescriptorSetLayoutCreateInfo layout_info{};
    layout_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layout_info.bindingCount = 1;
    layout_info.pBindings = &layout_binding;
    if (vkCreateDescriptorSetLayout(device, 
                                    &layout_info, 
                                    NULL, 
                                    &descriptor_set_layout) != VK_SUCCESS) {
        printf("Failed to create descriptor set layout\n");
        return 1;
    }
    return 0;
}

Err create_graphics_pipeline() 
{
    VkShaderModule vert_shader;
    VkShaderModule frag_shader;
    i32 len;
    char* buffer = read_file("../vert.spv", &len);
    if (!buffer)
        return 10;
    if (create_shader_module(buffer, len, &vert_shader))
        return 5;
    free(buffer);
    buffer = read_file("../frag.spv", &len);
    if (!buffer)
        return 10;
    if (create_shader_module(buffer, len, &frag_shader))
        return 6;
    free(buffer);
    VkPipelineShaderStageCreateInfo vert_create_info{};
    vert_create_info.sType =
        VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vert_create_info.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vert_create_info.module = vert_shader;
    vert_create_info.pName = "main";
    VkPipelineShaderStageCreateInfo frag_create_info{};
    frag_create_info.sType =
        VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    frag_create_info.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    frag_create_info.module = frag_shader;
    frag_create_info.pName = "main";
    VkPipelineShaderStageCreateInfo shader_stages[] = {
        vert_create_info,
        frag_create_info
    };
    VkPipelineDynamicStateCreateInfo dynamic_state{};
    dynamic_state.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamic_state.dynamicStateCount = 0;
    VkVertexInputBindingDescription binding_description = bind_desc();
    VkVertexInputAttributeDescription attribute_description[2]; 
    get_attr_desc(attribute_description);
    VkPipelineVertexInputStateCreateInfo vertex_input_info{};
    vertex_input_info.sType =
        VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertex_input_info.vertexBindingDescriptionCount = 1;
    vertex_input_info.pVertexBindingDescriptions = &binding_description;
    // TODO: this has to be set correctly
    vertex_input_info.vertexAttributeDescriptionCount = 2;
    vertex_input_info.pVertexAttributeDescriptions = attribute_description;
    VkPipelineInputAssemblyStateCreateInfo input_assembly{};
    input_assembly.sType =
        VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    input_assembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    input_assembly.primitiveRestartEnable = VK_FALSE;
    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = (float)swap_chain_extent.width;
    viewport.height = (float)swap_chain_extent.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    VkRect2D scissors{};
    scissors.offset = {0, 0};
    scissors.extent = swap_chain_extent;
    VkPipelineViewportStateCreateInfo viewport_state{};
    viewport_state.sType =
        VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewport_state.viewportCount = 1;
    viewport_state.scissorCount = 1;
    viewport_state.pViewports = &viewport;
    viewport_state.pScissors = &scissors;
    VkPipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.sType =
        VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterizer.depthBiasEnable = VK_FALSE;
    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType =
        VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    VkPipelineColorBlendAttachmentState color_blend_attachment{};
    color_blend_attachment.colorWriteMask =
        VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
        VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    color_blend_attachment.blendEnable = false;
    VkPipelineColorBlendStateCreateInfo color_blending{};
    color_blending.sType =
        VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    color_blending.logicOpEnable = VK_FALSE;
    color_blending.attachmentCount = 1;
    color_blending.pAttachments = &color_blend_attachment;
    VkPipelineLayoutCreateInfo pipeline_layout_info{};
    pipeline_layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipeline_layout_info.setLayoutCount = 1;
    pipeline_layout_info.pSetLayouts = &descriptor_set_layout;
    if (vkCreatePipelineLayout(device, &pipeline_layout_info, NULL,
                               &pipeline_layout) != VK_SUCCESS) {
        printf("Failed to create pipeline layout\n");
        return 1;
    }
    VkPipelineDepthStencilStateCreateInfo depth_stencil{};
    depth_stencil.sType = 
        VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depth_stencil.depthTestEnable = VK_TRUE;
    depth_stencil.depthWriteEnable = VK_TRUE;
    depth_stencil.depthCompareOp = VK_COMPARE_OP_LESS;
    depth_stencil.depthBoundsTestEnable = VK_FALSE;
    depth_stencil.minDepthBounds = 0.0f;
    depth_stencil.maxDepthBounds = 1.0f;
    depth_stencil.stencilTestEnable = VK_FALSE;
    VkGraphicsPipelineCreateInfo pipeline_info{};
    pipeline_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipeline_info.stageCount = 2;
    pipeline_info.pStages = shader_stages;
    pipeline_info.pVertexInputState = &vertex_input_info;
    pipeline_info.pInputAssemblyState = &input_assembly;
    pipeline_info.pViewportState = &viewport_state;
    pipeline_info.pRasterizationState = &rasterizer;
    pipeline_info.pMultisampleState = &multisampling;
    pipeline_info.pDepthStencilState = &depth_stencil;
    pipeline_info.pColorBlendState = &color_blending;
    pipeline_info.pDynamicState = &dynamic_state;
    pipeline_info.layout = pipeline_layout;
    pipeline_info.renderPass = render_pass;
    pipeline_info.subpass = 0;
    pipeline_info.basePipelineHandle = VK_NULL_HANDLE;
    pipeline_info.basePipelineIndex = -1;
    if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipeline_info,
                                  NULL, &graphics_pipeline) != VK_SUCCESS) {
        printf("Failed to create graphics pipeline\n");
        return 2;
    }
    vkDestroyShaderModule(device, vert_shader, NULL);
    vkDestroyShaderModule(device, frag_shader, NULL);
    return 0;
}

VkFormat find_supported_format(VkFormat* candidates, 
                               u32 candidate_count, 
                               VkImageTiling tiling,
                               VkFormatFeatureFlags features)
{
    for (u32 i = 0; i < candidate_count; ++i) {
        VkFormat format = candidates[i];
        VkFormatProperties props;
        vkGetPhysicalDeviceFormatProperties(physical_device, format, &props);
        if (tiling == VK_IMAGE_TILING_LINEAR && 
            (props.linearTilingFeatures & features) == features) {
            return format;
        } else if (tiling == VK_IMAGE_TILING_OPTIMAL && 
            (props.optimalTilingFeatures & features) == features) {
            return format;
        }
    }
    printf("Failed to find supported format\n");
    return VK_FORMAT_D32_SFLOAT;
}

VkFormat find_depth_format() 
{
    VkFormat formats[] = {
        VK_FORMAT_D32_SFLOAT,
        VK_FORMAT_D32_SFLOAT_S8_UINT,
        VK_FORMAT_D24_UNORM_S8_UINT
    };
    return find_supported_format(formats, 
                            3, 
                            VK_IMAGE_TILING_OPTIMAL, 
                            VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
}

Err create_render_pass() 
{
    VkAttachmentDescription color_attachment{};
    color_attachment.format = swap_chain_image_format;
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
    VkAttachmentDescription depth_attachment{};
    depth_attachment.format = find_depth_format();
    depth_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
    depth_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depth_attachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depth_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depth_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depth_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    depth_attachment.finalLayout = 
        VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    VkAttachmentReference depth_attachment_ref{};
    depth_attachment_ref.attachment = 1;
    depth_attachment_ref.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &color_attachment_ref;
    subpass.pDepthStencilAttachment = &depth_attachment_ref;
    VkAttachmentDescription attachments[] = {
        color_attachment,
        depth_attachment
    };
    VkRenderPassCreateInfo render_pass_info{};
    render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    render_pass_info.attachmentCount = 2;
    render_pass_info.pAttachments = attachments;
    render_pass_info.subpassCount = 1;
    render_pass_info.pSubpasses = &subpass;
    VkSubpassDependency dependency{};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT |
                                VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT |
                                VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT |
                                VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    render_pass_info.dependencyCount = 1;
    render_pass_info.pDependencies = &dependency;
    if (vkCreateRenderPass(device, &render_pass_info, NULL, &render_pass) !=
        VK_SUCCESS) {
        printf("Failed to create render pass!\n");
        return 1;
    }
    return 0;
}

Err create_framebuffers() 
{
    swap_chain_framebuffers.resize(swap_chain_image_views.size());
    for (size_t i = 0; i < swap_chain_image_views.size(); i++) {
        VkImageView attachments[] = {
            swap_chain_image_views[i],
            depth_image_view
        };
        VkFramebufferCreateInfo framebuffer_info{};
        framebuffer_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebuffer_info.renderPass = render_pass;
        framebuffer_info.attachmentCount = 2;
        framebuffer_info.pAttachments = attachments;
        framebuffer_info.width = swap_chain_extent.width;
        framebuffer_info.height = swap_chain_extent.height;
        framebuffer_info.layers = 1;
        if (vkCreateFramebuffer(device, &framebuffer_info, NULL,
                                &swap_chain_framebuffers[i]) != VK_SUCCESS) {
            printf("Failed to create framebuffer\n");
            return 1;
        }
    }
    return 0;
}

Err create_command_pool() 
{
    VkCommandPoolCreateInfo pool_info{};
    pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    pool_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    pool_info.queueFamilyIndex = queue_indices.graphics;
    if (vkCreateCommandPool(device, &pool_info, NULL, &command_pool) !=
        VK_SUCCESS) {
        printf("Failed to create command pool\n");
        return 1;
    }
    return 0;
}

VkCommandBuffer begin_single_time_commands()
{
    VkCommandBufferAllocateInfo alloc_info{};
    alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    alloc_info.commandPool = command_pool;
    alloc_info.commandBufferCount = 1;
    VkCommandBuffer command_buffer;
    vkAllocateCommandBuffers(device, &alloc_info, &command_buffer);
    VkCommandBufferBeginInfo begin_info{};
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(command_buffer, &begin_info);
    return command_buffer;
}

void end_single_time_commands(VkCommandBuffer buffer) 
{
    vkEndCommandBuffer(buffer);
    VkSubmitInfo submit_info{};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &buffer;
    vkQueueSubmit(graphics_queue, 1, &submit_info, VK_NULL_HANDLE);
    vkQueueWaitIdle(graphics_queue);
    vkFreeCommandBuffers(device, command_pool, 1, &buffer);
}

void copy_buffer(VkBuffer src_buffer, VkBuffer dst_buffer, VkDeviceSize size) 
{
    VkCommandBuffer cmd_buffer = begin_single_time_commands();
    VkBufferCopy copy_region{};
    copy_region.srcOffset = 0;
    copy_region.dstOffset = 0;
    copy_region.size = size;
    vkCmdCopyBuffer(cmd_buffer, src_buffer, dst_buffer, 1, &copy_region);
    end_single_time_commands(cmd_buffer);
}

u32 find_memory_type(u32 type_filter, VkMemoryPropertyFlags properties) 
{
    VkPhysicalDeviceMemoryProperties mem_properties;
    vkGetPhysicalDeviceMemoryProperties(physical_device, &mem_properties);
    for (u32 i = 0; i < mem_properties.memoryTypeCount; i++) {
        if (type_filter & (1 << i) &&
            (mem_properties.memoryTypes[i].propertyFlags & properties) ==
            properties) {
            return i;
        }
    }
    // TODO: Error handling?
    printf("Failed to find suitable memory type\n");
    return 1000;
}

Err create_buffer(VkDeviceSize size, VkBufferUsageFlags usage,
                   VkMemoryPropertyFlags properties, VkBuffer &buffer,
                   VkDeviceMemory &buffer_memory) 
{
    VkBufferCreateInfo buffer_info{};
    buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    buffer_info.size = size;
    buffer_info.usage = usage;
    buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    if (vkCreateBuffer(device, &buffer_info, NULL, &buffer) != VK_SUCCESS) {
        printf("Failed to create vertex buffer\n");
        return 1;
    }
    VkMemoryRequirements mem_requirements;
    vkGetBufferMemoryRequirements(device, buffer, &mem_requirements);
    VkMemoryAllocateInfo alloc_info{};
    alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    alloc_info.allocationSize = mem_requirements.size;
    alloc_info.memoryTypeIndex =
        find_memory_type(mem_requirements.memoryTypeBits, properties);
    if (vkAllocateMemory(device, &alloc_info, NULL, &buffer_memory) !=
        VK_SUCCESS) {
        printf("Failed to allocate vertex buffer memory\n");
        return 2;
    }
    vkBindBufferMemory(device, buffer, buffer_memory, 0);
    return 0;
}

Err create_vertex_buffer() 
{
    VkDeviceSize buffer_size = vertex_count * sizeof(Vertex);
    VkBuffer staging_buffer;
    VkDeviceMemory staging_buffer_memory;
    if (create_buffer(buffer_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                        VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                        staging_buffer, staging_buffer_memory))
        return 1;
    void *data;
    vkMapMemory(device, staging_buffer_memory, 0, buffer_size, 0, &data);
    memcpy(data, vertices, buffer_size);
    vkUnmapMemory(device, staging_buffer_memory);
    if (create_buffer(buffer_size,
                        VK_BUFFER_USAGE_TRANSFER_DST_BIT |
                        VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, vertex_buffer,
                        vertex_buffer_memory))
        return 2;
    copy_buffer(staging_buffer, vertex_buffer, buffer_size);
    vkDestroyBuffer(device, staging_buffer, NULL);
    vkFreeMemory(device, staging_buffer_memory, NULL);
    return 0;
}

Err create_index_buffer() 
{
    VkDeviceSize buffer_size = index_count * sizeof(u32);
    VkBuffer staging_buffer;
    VkDeviceMemory staging_buffer_memory;
    if (create_buffer(buffer_size, 
                        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                        VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                        staging_buffer, 
                        staging_buffer_memory))
        return 1;
    void *data;
    vkMapMemory(device, staging_buffer_memory, 0, buffer_size, 0, &data);
    memcpy(data, indices, buffer_size);
    vkUnmapMemory(device, staging_buffer_memory);
    if (create_buffer(buffer_size,
            VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 
            index_buffer, 
            index_buffer_memory))
        return 2;
    copy_buffer(staging_buffer, index_buffer, buffer_size);
    vkDestroyBuffer(device, staging_buffer, NULL);
    vkFreeMemory(device, staging_buffer_memory, NULL);
    return 0;
}

Err create_uniform_buffer() 
{
    VkDeviceSize buffer_size = sizeof(UniformBufferObject);
    uniform_buffers.resize(max_frames_in_flight);
    uniform_buffers_memory.resize(max_frames_in_flight);
    uniform_buffers_mapped.resize(max_frames_in_flight);
    for (u32 i = 0; i < max_frames_in_flight; ++i) {
        ENSURE(create_buffer(buffer_size, 
                      VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | 
                             VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                      uniform_buffers[i],
                      uniform_buffers_memory[i]), 1)
        vkMapMemory(device, 
                    uniform_buffers_memory[i], 
                    0, 
                    buffer_size, 
                    0, 
                    &uniform_buffers_mapped[i]);
    }
    return 0;
}

Err create_descriptor_pool() 
{
    VkDescriptorPoolSize pool_size{};
    pool_size.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    pool_size.descriptorCount = (u32) max_frames_in_flight;
    VkDescriptorPoolCreateInfo pool_info{};
    pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    pool_info.poolSizeCount = 1;
    pool_info.pPoolSizes = &pool_size;
    pool_info.maxSets = (u32) max_frames_in_flight;
    if (vkCreateDescriptorPool(device, 
                               &pool_info, 
                               NULL, 
                               &descriptor_pool) != VK_SUCCESS) {
        printf("Failed to create descriptor pool\n");
        return 1;
    }
    return 0;
}

Err create_descriptor_sets() 
{
    std::vector<VkDescriptorSetLayout> layouts(max_frames_in_flight, 
                                               descriptor_set_layout);
    VkDescriptorSetAllocateInfo alloc_info{};
    alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    alloc_info.descriptorPool = descriptor_pool;
    alloc_info.descriptorSetCount = (u32) max_frames_in_flight;
    alloc_info.pSetLayouts = layouts.data();
    descriptor_sets.resize(max_frames_in_flight);
    if (vkAllocateDescriptorSets(device, 
                                 &alloc_info, 
                                 descriptor_sets.data()) != VK_SUCCESS) {
        printf("Failed to allocate descriptor sets\n");
        return 1;
    }
    for (u32 i = 0; i < max_frames_in_flight; ++i) {
        VkDescriptorBufferInfo buffer_info{};
        buffer_info.buffer = uniform_buffers[i];
        buffer_info.offset = 0;
        buffer_info.range = sizeof(UniformBufferObject);
        VkWriteDescriptorSet descriptor_write{};
        descriptor_write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptor_write.dstSet = descriptor_sets[i];
        descriptor_write.dstBinding = 0;
        descriptor_write.dstArrayElement = 0;
        descriptor_write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptor_write.descriptorCount = 1;
        descriptor_write.pBufferInfo = &buffer_info;
        descriptor_write.pImageInfo = NULL;
        descriptor_write.pTexelBufferView = NULL;
        vkUpdateDescriptorSets(device, 1, &descriptor_write, 0, NULL);
    }
    return 0;
}

void cleanup_swapchain() 
{
    vkDestroyImageView(device, depth_image_view, NULL);
    vkDestroyImage(device, depth_image, NULL);
    vkFreeMemory(device, depth_image_memory, NULL);
    for (VkFramebuffer framebuffer : swap_chain_framebuffers) {
        vkDestroyFramebuffer(device, framebuffer, NULL);
    }
    for (VkImageView image_view : swap_chain_image_views) {
        vkDestroyImageView(device, image_view, NULL);
    }
    vkDestroySwapchainKHR(device, swap_chain, NULL);
}

void create_depth_resources();

Err recreate_swap_chain() 
{
    i32 width = 0;
    i32 height = 0;
    glfwGetFramebufferSize(window, &width, &height);
    while (width == 0 || height == 0) {
        glfwGetFramebufferSize(window, &width, &height);
        glfwWaitEvents();
    }
    vkDeviceWaitIdle(device);
    cleanup_swapchain();
    if (create_swap_chain())
        return 1;
    if (create_image_views())
        return 2;
    create_depth_resources();
    if (create_framebuffers())
        return 3;
    return 0;
}

Err create_command_buffers() 
{
    command_buffers.resize(max_frames_in_flight);
    VkCommandBufferAllocateInfo alloc_info{};
    alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    alloc_info.commandPool = command_pool;
    alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    alloc_info.commandBufferCount = (u32) command_buffers.size();
    if (vkAllocateCommandBuffers(device, &alloc_info, command_buffers.data()) !=
        VK_SUCCESS) {
        printf("Failed to allocate command buffers\n");
        return 1;
    }
    return 0;
}

Err create_sync_objects() 
{
    image_available_semaphores.resize(max_frames_in_flight);
    render_finished_semaphores.resize(max_frames_in_flight);
    in_flight_fences.resize(max_frames_in_flight);
    VkSemaphoreCreateInfo semaphore_info{};
    semaphore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    VkFenceCreateInfo fence_info{};
    fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fence_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;
    for (size_t i = 0; i < max_frames_in_flight; i++) {
        if (vkCreateSemaphore(device, &semaphore_info, NULL,
                              &image_available_semaphores[i]) ||
            vkCreateSemaphore(device, &semaphore_info, NULL,
                              &render_finished_semaphores[i]) ||
            vkCreateFence(device, &fence_info, NULL, &in_flight_fences[i]) !=
            VK_SUCCESS) {
            printf("Failed to create semaphore\n");
            return 1;
        }
    }
    return 0;
}

Err load_assets() 
{
    i32 file_len;
    char* buffer = read_file("../models/PM3D_Cube3D2.mod", &file_len);
    if (!buffer)
        return 1;

    u32* int_ptr = (u32*) buffer;
    vertex_count = *int_ptr;
    ++int_ptr;
    index_count = *int_ptr;
    ++int_ptr;
    
    vertices = (Vertex*) malloc(sizeof(Vertex) * vertex_count);
    indices = (u32*) malloc(sizeof(u32) * index_count);

    float* float_ptr = (float*) int_ptr;
    for (u32 i = 0; i < vertex_count; ++i) {
        Vertex vertex;
        vertex.x = *(float_ptr++);
        vertex.y = *(float_ptr++);
        vertex.z = *(float_ptr++);
        vertex.r = *(float_ptr++);
        vertex.g = *(float_ptr++);
        vertex.b = *(float_ptr++);
        vertices[i] = vertex;
    }
    int_ptr = (u32*) float_ptr;
    for (u32 i = 0; i < index_count; ++i) {
        indices[i] = *(int_ptr++);
    }
    free(buffer);
    return 0;
}

bool has_stencil_component(VkFormat format)
{
    return format == VK_FORMAT_D32_SFLOAT_S8_UINT || 
            format == VK_FORMAT_D24_UNORM_S8_UINT;
}

void transition_image_layout(VkImage image, 
                             VkFormat format, 
                             VkImageLayout old_layout, 
                             VkImageLayout new_layout)
{
    VkCommandBuffer cmd_buffer = begin_single_time_commands();
    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = old_layout;
    barrier.newLayout = new_layout;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = image;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;
    if (new_layout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
        if (has_stencil_component(format)) {
            barrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
        }
    } else {
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    }
    VkPipelineStageFlags source_stage;
    VkPipelineStageFlags destination_stage;
    if (old_layout == VK_IMAGE_LAYOUT_UNDEFINED && 
            new_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        source_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destination_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    } else if (old_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && 
            new_layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        source_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        destination_stage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    } else if (old_layout == VK_IMAGE_LAYOUT_UNDEFINED &&
            new_layout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT |
            VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    }else {
        printf("Unsupported layout transition\n");
        return;
    }
    vkCmdPipelineBarrier(cmd_buffer,
                         source_stage,
                         destination_stage,
                         0,
                         0, NULL,
                         0, NULL,
                         1, &barrier);
    end_single_time_commands(cmd_buffer);
}

Err create_image(u32 width, 
                  u32 height, 
                  VkFormat format, 
                  VkImageTiling tiling,
                  VkImageUsageFlags usage,
                  VkMemoryPropertyFlags properties,
                  VkImage* image,
                  VkDeviceMemory* image_memory) 
{
    VkImageCreateInfo image_info{};
    image_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    image_info.imageType = VK_IMAGE_TYPE_2D;
    image_info.extent.width = width;
    image_info.extent.height = height;
    image_info.extent.depth = 1;
    image_info.mipLevels = 1;
    image_info.arrayLayers = 1;
    image_info.format = format;
    image_info.tiling = tiling;
    image_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    image_info.usage = usage;
    image_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    image_info.samples = VK_SAMPLE_COUNT_1_BIT;
    image_info.flags = 0;
    if (vkCreateImage(device, 
                      &image_info, 
                      NULL, 
                      image) != VK_SUCCESS) {
        printf("Failed to create image\n");
        return 2;
    }
    VkMemoryRequirements mem_requirements;
    vkGetImageMemoryRequirements(device, *image, &mem_requirements);
    VkMemoryAllocateInfo alloc_info{};
    alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    alloc_info.allocationSize = mem_requirements.size;
    alloc_info.memoryTypeIndex = find_memory_type(
        mem_requirements.memoryTypeBits,
        properties);
    if (vkAllocateMemory(device, 
                         &alloc_info, 
                         NULL, 
                         image_memory) != VK_SUCCESS) {
        printf("Failed to allocate image memory\n");
        return 3;
    }
    vkBindImageMemory(device, *image, *image_memory, 0);
    return 0;
}


void create_depth_resources()
{
    VkFormat depth_format = find_depth_format();
    create_image(swap_chain_extent.width, 
                 swap_chain_extent.height,
                 depth_format,
                 VK_IMAGE_TILING_OPTIMAL,
                 VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
                 VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                 &depth_image,
                 &depth_image_memory);
    depth_image_view = create_image_view(depth_image, 
                                         depth_format, 
                                         VK_IMAGE_ASPECT_DEPTH_BIT);
    transition_image_layout(depth_image, 
                            depth_format, 
                            VK_IMAGE_LAYOUT_UNDEFINED,
                            VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
}

void copy_buffer_to_image(VkBuffer buffer, 
                          VkImage image, 
                          u32 width, 
                          u32 height) 
{
    VkCommandBuffer cmd_buffer = begin_single_time_commands();
    VkBufferImageCopy region{};
    region.bufferOffset = 0;
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;
    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1;
    region.imageOffset = {0, 0, 0};
    region.imageExtent = {
        width,
        height,
        1
    };
    vkCmdCopyBufferToImage(cmd_buffer,
                           buffer,
                           image,
                           VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                           1,
                           &region);
    end_single_time_commands(cmd_buffer);
}

Err create_texture_image()
{
    i32 tex_width;
    i32 tex_height;
    i32 tex_channels;
    const char* tex_path = "../textures/texture.jpg";
    stbi_uc* pixels = stbi_load(tex_path, 
                                &tex_width, 
                                &tex_height, 
                                &tex_channels, 
                                STBI_rgb_alpha);
    VkDeviceSize image_size = tex_width * tex_height * 4;
    if (!pixels) {
        printf("Failed to load texture image: %s\n", tex_path);
        return 1;
    }
    VkBuffer staging_buffer;
    VkDeviceMemory staging_buffer_memory;
    create_buffer(image_size, 
                  VK_BUFFER_USAGE_TRANSFER_SRC_BIT, 
                  VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | 
                  VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, 
                  staging_buffer, 
                  staging_buffer_memory);
    void* data = NULL;
    vkMapMemory(device, staging_buffer_memory, 0, image_size, 0, &data);
    memcpy(data, pixels, image_size);
    vkUnmapMemory(device, staging_buffer_memory);
    stbi_image_free(pixels);
    create_image(tex_width, 
                 tex_height, 
                 VK_FORMAT_R8G8B8A8_SRGB, 
                 VK_IMAGE_TILING_OPTIMAL, 
                 VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, 
                 VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 
                 &texture_image, 
                 &texture_image_memory);
    transition_image_layout(texture_image, 
                            VK_FORMAT_R8G8B8A8_SRGB, 
                            VK_IMAGE_LAYOUT_UNDEFINED, 
                            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    copy_buffer_to_image(staging_buffer, 
                         texture_image, 
                         tex_width, 
                         tex_height);
    transition_image_layout(texture_image, 
                            VK_FORMAT_R8G8B8A8_SRGB, 
                            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 
                            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    vkDestroyBuffer(device, staging_buffer, NULL);
    vkFreeMemory(device, staging_buffer_memory, NULL);
    return 0;
}

void create_texture_image_view()
{
    texture_image_view = create_image_view(texture_image, 
                                           VK_FORMAT_R8G8B8A8_SRGB,
                                           VK_IMAGE_ASPECT_COLOR_BIT);
}

void create_texture_sampler()
{
    VkSamplerCreateInfo sampler_info{};
    sampler_info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    sampler_info.magFilter = VK_FILTER_LINEAR;
    sampler_info.minFilter = VK_FILTER_LINEAR;
    sampler_info.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    sampler_info.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    sampler_info.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    sampler_info.anisotropyEnable = VK_TRUE;
    VkPhysicalDeviceProperties properties{};
    vkGetPhysicalDeviceProperties(physical_device, &properties);
    sampler_info.maxAnisotropy = properties.limits.maxSamplerAnisotropy;
    sampler_info.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    sampler_info.unnormalizedCoordinates = VK_FALSE;
    sampler_info.compareEnable = VK_FALSE;
    sampler_info.compareOp = VK_COMPARE_OP_ALWAYS;
    sampler_info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    sampler_info.mipLodBias = 0.0f;
    sampler_info.minLod = 0.0f;
    sampler_info.maxLod = 0.0f;
    if (vkCreateSampler(device, 
                        &sampler_info, 
                        NULL, 
                        &texture_sampler) != VK_SUCCESS) {
        // TODO: handle this one
        printf("Failed to create texture sampler\n");
    }
}

Err init_vulkan() 
{
    ENSURE(create_instance(), 1);
    ENSURE(create_surface(), 2);
    ENSURE(pick_physical_device(&physical_device), 3);
    ENSURE(create_logical_device(), 4);
    ENSURE(create_swap_chain(), 5);
    ENSURE(create_image_views(), 6);
    ENSURE(create_render_pass(), 7);
    ENSURE(create_descriptor_set_layout(), 15);
    ENSURE(create_graphics_pipeline(), 8);
    ENSURE(create_command_pool(), 10);
    create_depth_resources();
    ENSURE(create_framebuffers(), 9);
    ENSURE(create_texture_image(), 20);
    create_texture_image_view();
    create_texture_sampler();
    ENSURE(create_vertex_buffer(), 11);
    ENSURE(create_index_buffer(), 12);
    ENSURE(create_uniform_buffer(), 16);
    ENSURE(create_descriptor_pool(), 17);
    ENSURE(create_descriptor_sets(), 18);
    ENSURE(create_command_buffers(), 13);
    ENSURE(create_sync_objects(), 14);

    return 0;
}

Err record_command_buffer(VkCommandBuffer buffer, u32 image_index) 
{
    VkCommandBufferBeginInfo begin_info{};
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    begin_info.flags = 0;
    begin_info.pInheritanceInfo = NULL;
    if (vkBeginCommandBuffer(buffer, &begin_info) != VK_SUCCESS) {
        printf("Failed to begin recording command buffer\n");
        return 1;
    }
    VkRenderPassBeginInfo render_pass_info{};
    render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    render_pass_info.renderPass = render_pass;
    render_pass_info.framebuffer = swap_chain_framebuffers[image_index];
    render_pass_info.renderArea.offset = {0, 0};
    render_pass_info.renderArea.extent = swap_chain_extent;
    VkClearValue clear_values[] = {
        {{0.0f, 0.0f, 0.0f, 1.0f}},     // Color buffer
        {1.0f, 0}                       // Depth buffer
    };
    render_pass_info.clearValueCount = 2;
    render_pass_info.pClearValues = clear_values;

    vkCmdBeginRenderPass(buffer, 
                         &render_pass_info, 
                         VK_SUBPASS_CONTENTS_INLINE);
    vkCmdBindPipeline(buffer, 
                      VK_PIPELINE_BIND_POINT_GRAPHICS,
                      graphics_pipeline);
    VkBuffer vertex_buffers[] = {vertex_buffer};
    VkDeviceSize offsets[] = {0};
    vkCmdBindVertexBuffers(buffer, 0, 1, vertex_buffers, offsets);
    vkCmdBindIndexBuffer(buffer, index_buffer, 0, VK_INDEX_TYPE_UINT32);
    vkCmdBindDescriptorSets(buffer, 
                            VK_PIPELINE_BIND_POINT_GRAPHICS, 
                            pipeline_layout,
                            0,
                            1,
                            &descriptor_sets[current_frame],
                            0,
                            NULL);
    vkCmdDrawIndexed(buffer, index_count, 1, 0, 0, 0);
    vkCmdEndRenderPass(buffer);
    if (vkEndCommandBuffer(buffer) != VK_SUCCESS) {
        printf("Failed to record command buffer\n");
        return 2;
    }
    return 0;
}

void update_uniform_buffer() 
{
    float time = glfwGetTime();

    UniformBufferObject ubo;
    ubo.model = glm::rotate(glm::mat4(1.0), 
                            glm::radians(90.f), 
                            glm::vec3(1.0f, 0.0f, 0.0f));
    ubo.model = glm::rotate(ubo.model, 
                            time * glm::radians(90.f), 
                            glm::vec3(0.0f, 1.0f, 0.0f));
    ubo.view = glm::lookAt(glm::vec3(4.0, 4.0, 4.0), 
                           glm::vec3(0.0, 0.0, 2.0), 
                           glm::vec3(0.0, 0.0, 1.0));
    ubo.proj = glm::perspective(glm::radians(45.0f), 
            (float) swap_chain_extent.width / (float) swap_chain_extent.height, 
            0.1f, 
            10.0f);
    ubo.proj[1][1] *= -1;

    memcpy(uniform_buffers_mapped[current_frame], &ubo, sizeof(ubo));
}

void draw_frame() 
{
    vkWaitForFences(device, 
                    1, 
                    &in_flight_fences[current_frame], 
                    VK_TRUE,
                    UINT64_MAX);
    u32 image_index;
    VkResult result = vkAcquireNextImageKHR(device, 
                                    swap_chain, 
                                    UINT64_MAX,
                                    image_available_semaphores[current_frame],
                                    VK_NULL_HANDLE, 
                                    &image_index);
    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
        recreate_swap_chain();
        return;
    } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
        printf("Failed to acquire swap chain image\n");
        return;
    }
    vkResetFences(device, 1, &in_flight_fences[current_frame]);
    vkResetCommandBuffer(command_buffers[current_frame], 0);
    update_uniform_buffer();
    record_command_buffer(command_buffers[current_frame], image_index);
    VkSubmitInfo submit_info{};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    VkSemaphore wait_semaphores[] = {
        image_available_semaphores[current_frame]
    };
    VkPipelineStageFlags wait_stages[] = {
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
    };
    submit_info.waitSemaphoreCount = 1;
    submit_info.pWaitSemaphores = wait_semaphores;
    submit_info.pWaitDstStageMask = wait_stages;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &command_buffers[current_frame];
    VkSemaphore signal_semaphores[] = {
        render_finished_semaphores[current_frame]
    };
    submit_info.signalSemaphoreCount = 1;
    submit_info.pSignalSemaphores = signal_semaphores;
    if (vkQueueSubmit(graphics_queue, 
                      1, 
                      &submit_info,
                      in_flight_fences[current_frame]) != VK_SUCCESS) {
        printf("Failed to submit draw command buffer\n");
        return;
    }
    VkPresentInfoKHR present_info{};
    present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    present_info.waitSemaphoreCount = 1;
    present_info.pWaitSemaphores = signal_semaphores;
    VkSwapchainKHR swap_chains[] = {swap_chain};
    present_info.swapchainCount = 1;
    present_info.pSwapchains = swap_chains;
    present_info.pImageIndices = &image_index;
    present_info.pResults = NULL;
    result = vkQueuePresentKHR(present_queue, &present_info);
    if (result == VK_ERROR_OUT_OF_DATE_KHR || 
        result == VK_SUBOPTIMAL_KHR ||
        frame_buffer_resized) {
        frame_buffer_resized = 0;
        recreate_swap_chain();
    } else if (result != VK_SUCCESS) {
        printf("Failed to present swap chain image\n");
        return;
    }
}

void main_loop() 
{
    current_frame = 0;
    while (!glfwWindowShouldClose(window)) {
        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
            glfwSetWindowShouldClose(window, true);
        }

        glfwPollEvents();
        draw_frame();
        current_frame = (current_frame + 1) % max_frames_in_flight;
    }
    vkDeviceWaitIdle(device);
}

void cleanup() 
{
    cleanup_swapchain();
    vkDestroySampler(device, texture_sampler, NULL);
    vkDestroyImageView(device, texture_image_view, NULL);
    vkDestroyImage(device, texture_image, NULL);
    vkFreeMemory(device, texture_image_memory, NULL);
    for (u32 i = 0; i < max_frames_in_flight; ++i) {
        vkDestroyBuffer(device, uniform_buffers[i], NULL);
        vkFreeMemory(device, uniform_buffers_memory[i], NULL);
    }
    vkDestroyDescriptorPool(device, descriptor_pool, NULL);
    vkDestroyDescriptorSetLayout(device, descriptor_set_layout, NULL);
    vkDestroyBuffer(device, vertex_buffer, NULL);
    vkFreeMemory(device, vertex_buffer_memory, NULL);
    vkDestroyBuffer(device, index_buffer, NULL);
    vkFreeMemory(device, index_buffer_memory, NULL);
    for (u32 i = 0; i < max_frames_in_flight; ++i) {
        vkDestroySemaphore(device, image_available_semaphores[i], NULL);
        vkDestroySemaphore(device, render_finished_semaphores[i], NULL);
        vkDestroyFence(device, in_flight_fences[i], NULL);
    }
    vkDestroyCommandPool(device, command_pool, NULL);
    vkDestroyPipeline(device, graphics_pipeline, NULL);
    vkDestroyPipelineLayout(device, pipeline_layout, NULL);
    vkDestroyRenderPass(device, render_pass, NULL);
    vkDestroyDevice(device, NULL);
    vkDestroySurfaceKHR(instance, surface, NULL);
    vkDestroyInstance(instance, NULL);
    glfwDestroyWindow(window);
    glfwTerminate();
}

i32 main() 
{
    init_window();
    load_assets();
    init_vulkan();
    main_loop();
    cleanup();
}
