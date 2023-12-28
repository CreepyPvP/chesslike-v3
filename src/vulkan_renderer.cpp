#include "include/vulkan_renderer.h"

#define STB_IMAGE_IMPLEMENTATION
#include "include/stb_image.h"
#include "include/utils.h"
#include "include/assets.h"
#include "include/arena.h"
#include "include/loading.h"
#include "include/game_math.h"
#include "include/render_queue.h"

#include <math.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <vector>

#define QUEUE_FAMILY_GRAPHICS 1 << 0
#define QUEUE_FAMILY_PRESENT 1 << 1

#define UNIFORM_BUF_GLOBAL 1
#define UNIFORM_BUF_MATERIAL 5
#define UNIFORM_BUF_OBJECT 10
#define UNIFORM_BUF_BONE 20

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


struct GlobalUniform
{
    glm::mat4 proj_view;
    glm::vec3 camera_pos;
    glm::vec2 screen_size;
    i32 jitter_index;
};

struct MaterialUniform
{
    float roughness;
    float smoothness;
    glm::vec3 specular;
    glm::vec3 diffuse;
};

struct ObjectUniform
{
    glm::mat4 model;
    glm::mat4 prev_mvp;
};

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

QueueFamilyIndices queue_indices;
VkInstance instance;
VkSurfaceKHR surface;
VkSwapchainKHR swap_chain;
std::vector<VkImage> swap_chain_images;
std::vector<VkImageView> swap_chain_image_views;
u32 image_count;
VkFormat swap_chain_image_format;
VkExtent2D swap_chain_extent;
VkDevice device;
VkPhysicalDevice physical_device;
VkQueue graphics_queue;
VkQueue present_queue;
VkRenderPass render_pass;
VkCommandPool command_pool;
std::vector<VkCommandBuffer> command_buffers;
std::vector<VkSemaphore> image_available_semaphores;
std::vector<VkSemaphore> render_finished_semaphores;
std::vector<VkFence> in_flight_fences;
u8 frame_buffer_resized;
VkDescriptorPool descriptor_pool;
u32 current_frame;
VkImage depth_image;
VkDeviceMemory depth_image_memory;
VkImageView depth_image_view;
VkImage texture_image;
VkDeviceMemory texture_image_memory;
VkImageView texture_image_view;
VkSampler texture_sampler;

// 0 => global, 1 => material, 2 => object
u32 dynamic_align[3];
u32 bone_stride;
u32 non_coherent_atom_size;
u32 object_offset;
u32 bone_offset;
u32 material_offset;

u32 uniform_object_alloc;
u32 uniform_bone_alloc;

u32 range_count = 0;
VkMappedMemoryRange ranges[UNIFORM_BUF_OBJECT + UNIFORM_BUF_MATERIAL + 2];
VkDescriptorSet descriptor_sets[max_frames_in_flight * 4];
VkDescriptorSetLayout descriptor_set_layouts[4];
std::vector<VkBuffer> uniform_buffers;
std::vector<VkDeviceMemory> uniform_buffers_memory;
std::vector<void*> uniform_buffers_mapped;

// render targets
VkImage render_images[max_frames_in_flight];
VkImageView render_image_views[max_frames_in_flight];
VkDeviceMemory render_image_memory[max_frames_in_flight];
VkFormat render_image_format = VK_FORMAT_R8G8B8A8_UNORM;
VkFramebuffer framebuffers[max_frames_in_flight];

// pipelines
// 0 => static objects, 1 => skinned objects
#define PIPELINE_COUNT 2
VkPipelineLayout pipeline_layouts[PIPELINE_COUNT];
VkPipeline graphics_pipelines[PIPELINE_COUNT];
VkBuffer vertex_buffer[PIPELINE_COUNT];
VkDeviceMemory vertex_buffer_memory[PIPELINE_COUNT];
VkBuffer index_buffer[PIPELINE_COUNT];
VkDeviceMemory index_buffer_memory[PIPELINE_COUNT];

RenderQueue render_queue;

// taa stuff...
glm::mat4 proj_view;
i32 jitter_index = 0;


bool is_complete(QueueFamilyIndices* self) 
{
    return self->flags & (QUEUE_FAMILY_GRAPHICS | QUEUE_FAMILY_PRESENT);
}

bool check_validation_layer_support() 
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
            return false;
        }
    }
    return true;
}

void create_instance() 
{
    if (!check_validation_layer_support()) {
        printf("Validation layers requested, but not available\n");
        exit(1);
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
        exit(1);
    }
}

void create_surface(GLFWwindow* window) 
{
    if (glfwCreateWindowSurface(instance, window, NULL, &surface) != VK_SUCCESS) {
        printf("Failed to create window surface\n");
        exit(1);
    }
}


bool check_device_extension_support(VkPhysicalDevice device) 
{
    u32 extension_count;
    vkEnumerateDeviceExtensionProperties(device, NULL, &extension_count, NULL);
    VkExtensionProperties* available_extensions = (VkExtensionProperties*) 
        malloc(sizeof(VkExtensionProperties) * extension_count);
    vkEnumerateDeviceExtensionProperties(device, NULL, &extension_count, available_extensions);
    for (u32 i = 0; i < device_extension_count; ++i) {
        bool found = false;
        for (u32 j = 0; j < extension_count; ++j) {
            if (strcmp(device_extensions[i], available_extensions[j].extensionName) == 0) {
                found = true;
                break;
            }
        }
        if (!found) {
            return false;
        }
    }
    return true;
}

// TODO: Get rid of all that malloc! Use arenas instead!
SwapChainSupportDetails query_swap_chain_support(VkPhysicalDevice device) 
{
    SwapChainSupportDetails details{};
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);
    u32 format_count;
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &format_count, NULL);
    if (format_count > 0) {
        details.formats = (VkSurfaceFormatKHR*)  malloc(sizeof(VkSurfaceFormatKHR) * format_count);
        details.format_count = format_count;
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &format_count,details.formats);
    }
    u32 present_mode_count;
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface,&present_mode_count, NULL);
    if (format_count > 0) {
        details.present_modes = (VkPresentModeKHR*) 
            malloc(sizeof(VkPresentModeKHR) * present_mode_count);
        details.present_count = present_mode_count;
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &present_mode_count, 
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
        vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &present_support);
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
    queue_indices = find_queue_families(device);
    bool extensions_support = check_device_extension_support(device);
    if (extensions_support) {
        SwapChainSupportDetails swap_chain_support = query_swap_chain_support(device);
        if (swap_chain_support.format_count == 0 ||
            swap_chain_support.present_count == 0) {
            return false;
        }
    }
    return is_complete(&queue_indices) && extensions_support;
}

void pick_physical_device(VkPhysicalDevice* device) 
{
    u32 device_count = 0;
    vkEnumeratePhysicalDevices(instance, &device_count, NULL);
    if (device_count == 0) {
        printf("Failed to find GPU with Vulkan support!\n");
        exit(1);
    }
    VkPhysicalDevice* devices = (VkPhysicalDevice*) malloc(sizeof(VkPhysicalDevice) * device_count);
    vkEnumeratePhysicalDevices(instance, &device_count, devices);
    for (u32 i = 0; i < device_count; ++i) {
        VkPhysicalDevice current = devices[i];
        if (is_device_suitable(current)) {
            *device = current;
            return;
        }
    }

    printf("Failed to find a suitable GPU\n");
    exit(1);
}

void create_logical_device() 
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
        exit(1);
    }
    vkGetDeviceQueue(device, queue_indices.graphics, 0, &graphics_queue);
    vkGetDeviceQueue(device, queue_indices.present, 0, &present_queue);
    return;
}

VkSurfaceFormatKHR choose_swap_surface_format(VkSurfaceFormatKHR* formats, u32 format_count)
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

VkPresentModeKHR choose_swap_present_mode(VkPresentModeKHR* present_modes, u32 present_mode_count) 
{
    for (u32 i = 0; i < present_mode_count; ++i) {
        VkPresentModeKHR present_mode = present_modes[i];
        if (present_mode == VK_PRESENT_MODE_MAILBOX_KHR) {
            return present_mode;
        }
    }
    return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D choose_swap_extent(const VkSurfaceCapabilitiesKHR* capabilities, GLFWwindow* window) 
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

void create_swap_chain(GLFWwindow* window) 
{
    SwapChainSupportDetails swap_chain_support = query_swap_chain_support(physical_device);
    VkSurfaceFormatKHR surface_format = choose_swap_surface_format(
                                    swap_chain_support.formats,
                                    swap_chain_support.format_count);
    VkPresentModeKHR present_mode = choose_swap_present_mode(
                                        swap_chain_support.present_modes, 
                                        swap_chain_support.present_count);
    VkExtent2D extent = choose_swap_extent(&swap_chain_support.capabilities, window);
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
    create_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    u32 queue_family_indices[] = { queue_indices.graphics, queue_indices.present };
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
        exit(1);
    }
    vkGetSwapchainImagesKHR(device, swap_chain, &image_count, NULL);
    swap_chain_images.resize(image_count);
    vkGetSwapchainImagesKHR(device, swap_chain, &image_count, swap_chain_images.data());
    swap_chain_image_format = surface_format.format;
    swap_chain_extent = extent;
}

VkImageView create_image_view(VkImage image, VkFormat format, VkImageAspectFlags aspect_flags) 
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
    if (vkCreateImageView(device, &view_info, NULL, &image_view) != VK_SUCCESS) {
        printf("Failed to create texture image view\n");
        exit(1);
    }
    return image_view;
}

  
u32 find_memory_type(u32 type_filter, VkMemoryPropertyFlags properties) 
{
    VkPhysicalDeviceMemoryProperties mem_properties;
    vkGetPhysicalDeviceMemoryProperties(physical_device, &mem_properties);
    for (u32 i = 0; i < mem_properties.memoryTypeCount; i++) {
        if (type_filter & (1 << i) &&
            (mem_properties.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }
    printf("Failed to find suitable memory type\n");
    exit(1);
}

void create_image(u32 width, 
                  u32 height, 
                  VkSampleCountFlagBits num_samples,
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
    image_info.samples = num_samples;
    image_info.flags = 0;
    if (vkCreateImage(device, &image_info, NULL, image) != VK_SUCCESS) {
        printf("Failed to create image\n");
        exit(1);
    }
    VkMemoryRequirements mem_requirements;
    vkGetImageMemoryRequirements(device, *image, &mem_requirements);
    VkMemoryAllocateInfo alloc_info{};
    alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    alloc_info.allocationSize = mem_requirements.size;
    alloc_info.memoryTypeIndex = find_memory_type(mem_requirements.memoryTypeBits, properties);
    if (vkAllocateMemory(device, &alloc_info, NULL, image_memory) != VK_SUCCESS) {
        printf("Failed to allocate image memory\n");
        exit(3);
    }
    vkBindImageMemory(device, *image, *image_memory, 0);
}

void create_render_images()
{
    for (u32 i = 0; i < max_frames_in_flight; ++i) {
        create_image(swap_chain_extent.width, swap_chain_extent.height,
                     VK_SAMPLE_COUNT_1_BIT, render_image_format,
                     VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | 
                     VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
                     VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                     render_images + i, render_image_memory + i);
    }
}

void create_image_views() 
{
    swap_chain_image_views.resize(swap_chain_images.size());
    for (u32 i = 0; i < swap_chain_images.size(); ++i) {
        swap_chain_image_views[i] = create_image_view(swap_chain_images[i], 
                                        swap_chain_image_format, VK_IMAGE_ASPECT_COLOR_BIT);
    }
    for (u32 i = 0; i < max_frames_in_flight; ++i) {
        render_image_views[i] = create_image_view(render_images[i],
                                        render_image_format, VK_IMAGE_ASPECT_COLOR_BIT);
    }
}

void create_shader_module(const char* code, u32 len, VkShaderModule* module) 
{
    VkShaderModuleCreateInfo create_info{};
    create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    create_info.codeSize = len;
    create_info.pCode = (u32*) code;
    if (vkCreateShaderModule(device, &create_info, NULL, module) != VK_SUCCESS) {
        printf("Failed to create shader module\n");
        exit(1);
    }
}

void create_layout(VkDescriptorSetLayoutBinding* bindings, 
                   u32 binding_count, 
                   VkDescriptorSetLayout* layout) 
{
    VkDescriptorSetLayoutCreateInfo layout_info{};
    layout_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layout_info.bindingCount = binding_count;
    layout_info.pBindings = bindings;
    if (vkCreateDescriptorSetLayout(device, &layout_info, NULL, layout) != VK_SUCCESS) {
        printf("Failed to create descriptor set layout\n");
        exit(1);
    }
}

void create_descriptor_set_layouts() 
{
    VkDescriptorSetLayoutBinding global_binding{};
    global_binding.binding = 0;
    global_binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    global_binding.descriptorCount = 1;
    global_binding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
    global_binding.pImmutableSamplers = NULL;

    VkDescriptorSetLayoutBinding sampler_binding{};
    sampler_binding.binding = 1;
    sampler_binding.descriptorCount = 1;
    sampler_binding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    sampler_binding.pImmutableSamplers = NULL;
    sampler_binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    VkDescriptorSetLayoutBinding material_binding{};
    material_binding.binding = 0;
    material_binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
    material_binding.descriptorCount = 1;
    material_binding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
    material_binding.pImmutableSamplers = NULL;

    VkDescriptorSetLayoutBinding object_binding{};
    object_binding.binding = 0;
    object_binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
    object_binding.descriptorCount = 1;
    object_binding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
    object_binding.pImmutableSamplers = NULL;

    VkDescriptorSetLayoutBinding bone_binding{};
    object_binding.binding = 0;
    object_binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    object_binding.descriptorCount = 1;
    object_binding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    object_binding.pImmutableSamplers = NULL;

    VkDescriptorSetLayoutBinding bindings[] = {
        global_binding,
        sampler_binding,
        material_binding,
        object_binding,
        bone_binding
    };

    create_layout(bindings, 2, descriptor_set_layouts);
    create_layout(bindings + 2, 1, descriptor_set_layouts + 1);
    create_layout(bindings + 3, 1, descriptor_set_layouts + 2);
    create_layout(bindings + 4, 1, descriptor_set_layouts + 3);
}

void create_graphics_pipeline(const char* vert_file,
                              const char* frag_file,
                              VkVertexInputAttributeDescription* attr_desc,
                              u32 attr_count,
                              VkVertexInputBindingDescription* bind_desc,
                              VkPipelineLayout* layout,
                              VkPipeline* pipeline)
{
    VkShaderModule vert_shader;
    VkShaderModule frag_shader;
    i32 len;
    char* buffer = read_file(vert_file, &len, NULL);
    if (!buffer)
        exit(1);
    create_shader_module(buffer, len, &vert_shader);
    free(buffer);
    buffer = read_file(frag_file, &len, NULL);
    if (!buffer)
        exit(1);
    create_shader_module(buffer, len, &frag_shader);
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
    VkPipelineVertexInputStateCreateInfo vertex_input_info{};
    vertex_input_info.sType =
        VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertex_input_info.vertexBindingDescriptionCount = 1;
    vertex_input_info.pVertexBindingDescriptions = bind_desc;
    vertex_input_info.vertexAttributeDescriptionCount = attr_count;
    vertex_input_info.pVertexAttributeDescriptions = attr_desc;
    VkPipelineInputAssemblyStateCreateInfo input_assembly{};
    input_assembly.sType =
        VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    input_assembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    input_assembly.primitiveRestartEnable = VK_FALSE;
    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = (float) swap_chain_extent.width;
    viewport.height = (float) swap_chain_extent.height;
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
    pipeline_layout_info.setLayoutCount = 3;
    pipeline_layout_info.pSetLayouts = descriptor_set_layouts;
    if (vkCreatePipelineLayout(device, &pipeline_layout_info, NULL,
                               layout) != VK_SUCCESS) {
        printf("Failed to create pipeline layout\n");
        exit(1);
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
    pipeline_info.layout = *layout;
    pipeline_info.renderPass = render_pass;
    pipeline_info.subpass = 0;
    pipeline_info.basePipelineHandle = VK_NULL_HANDLE;
    pipeline_info.basePipelineIndex = -1;
    if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipeline_info,
                                  NULL, pipeline) != VK_SUCCESS) {
        printf("Failed to create graphics pipeline\n");
        exit(1);
    }
    vkDestroyShaderModule(device, vert_shader, NULL);
    vkDestroyShaderModule(device, frag_shader, NULL);
}

void create_pipelines() 
{
    VkVertexInputBindingDescription static_bind_desc{};
    static_bind_desc.binding = 0;
    static_bind_desc.stride = sizeof(Vertex);
    static_bind_desc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    VkVertexInputBindingDescription rigged_bind_desc{};
    rigged_bind_desc.binding = 0;
    rigged_bind_desc.stride = sizeof(RiggedVertex);
    rigged_bind_desc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    VkVertexInputAttributeDescription static_attr[2];
    static_attr[0].binding = 0;
    static_attr[0].location = 0;
    static_attr[0].format = VK_FORMAT_R32G32B32_SFLOAT;
    static_attr[0].offset = offsetof(Vertex, x);
    static_attr[1].binding = 0;
    static_attr[1].location = 1;
    static_attr[1].format = VK_FORMAT_R32G32B32_SFLOAT;
    static_attr[1].offset = offsetof(Vertex, nx);

    VkVertexInputAttributeDescription rigged_attr[4];
    rigged_attr[0].binding = 0;
    rigged_attr[0].location = 0;
    rigged_attr[0].format = VK_FORMAT_R32G32B32_SFLOAT;
    rigged_attr[0].offset = offsetof(RiggedVertex, x);
    rigged_attr[1].binding = 0;
    rigged_attr[1].location = 1;
    rigged_attr[1].format = VK_FORMAT_R32G32B32_SFLOAT;
    rigged_attr[1].offset = offsetof(RiggedVertex, nx);
    rigged_attr[2].binding = 0;
    rigged_attr[2].location = 2;
    rigged_attr[2].format = VK_FORMAT_R32G32B32_SINT;
    rigged_attr[2].offset = offsetof(RiggedVertex, bones);
    rigged_attr[3].binding = 0;
    rigged_attr[3].location = 3;
    rigged_attr[3].format = VK_FORMAT_R32G32B32_SFLOAT;
    rigged_attr[3].offset = offsetof(RiggedVertex, weights);

    create_graphics_pipeline("shader/staticv.spv",
                             "shader/pbr_frag.spv",
                             static_attr,
                             2,
                             &static_bind_desc,
                             pipeline_layouts,
                             graphics_pipelines);

    create_graphics_pipeline("shader/skinnedv.spv",
                             "shader/pbr_frag.spv",
                             rigged_attr,
                             4,
                             &rigged_bind_desc,
                             pipeline_layouts + 1,
                             graphics_pipelines + 1);
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
    return find_supported_format(formats, 3, 
                                 VK_IMAGE_TILING_OPTIMAL, 
                                 VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
}

void create_render_pass() 
{
    VkAttachmentDescription color_attachment{};
    color_attachment.format = render_image_format;
    color_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
    color_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    color_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    color_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    color_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    color_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    color_attachment.finalLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    VkAttachmentReference color_attachment_ref{};
    color_attachment_ref.attachment = 0;
    // QUESTION: Unterschied zu color_attachment.finalLayout?
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
    depth_attachment_ref.layout = 
        VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &color_attachment_ref;
    subpass.pDepthStencilAttachment = &depth_attachment_ref;
    VkAttachmentDescription attachments[] = {
        color_attachment,
        depth_attachment,
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
        exit(1);
    }
}

void create_framebuffers() 
{
    for (u32 i = 0; i < max_frames_in_flight; ++i) {
        VkImageView attachments[] = {
            render_image_views[i],
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
        if (vkCreateFramebuffer(device, &framebuffer_info, NULL, framebuffers + i) != VK_SUCCESS) {
            printf("Failed to create framebuffer\n");
            exit(1);
        }
    }
}

void create_command_pool() 
{
    VkCommandPoolCreateInfo pool_info{};
    pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    pool_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    pool_info.queueFamilyIndex = queue_indices.graphics;
    if (vkCreateCommandPool(device, &pool_info, NULL, &command_pool) != VK_SUCCESS) {
        printf("Failed to create command pool\n");
        exit(1);
    }
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

void create_buffer(VkDeviceSize size, 
                  VkBufferUsageFlags usage,
                  VkMemoryPropertyFlags properties, 
                  VkBuffer* buffer,
                  VkDeviceMemory* buffer_memory) 
{
    VkBufferCreateInfo buffer_info{};
    buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    buffer_info.size = size;
    buffer_info.usage = usage;
    buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    if (vkCreateBuffer(device, &buffer_info, NULL, buffer) != VK_SUCCESS) {
        printf("Failed to create vertex buffer\n");
        exit(1);
    }
    VkMemoryRequirements mem_requirements;
    vkGetBufferMemoryRequirements(device, *buffer, &mem_requirements);
    VkMemoryAllocateInfo alloc_info{};
    alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    alloc_info.allocationSize = mem_requirements.size;
    alloc_info.memoryTypeIndex = find_memory_type(mem_requirements.memoryTypeBits, properties);
    if (vkAllocateMemory(device, &alloc_info, NULL, buffer_memory) !=
        VK_SUCCESS) {
        printf("Failed to allocate vertex buffer memory\n");
        exit(1);
    }
    vkBindBufferMemory(device, *buffer, *buffer_memory, 0);
}

void create_vertex_buffer(VkBuffer* buffer, VkDeviceMemory* memory, Arena* arena) 
{
    VkDeviceSize buffer_size = arena->size;
    VkBuffer staging_buffer;
    VkDeviceMemory staging_buffer_memory;
    create_buffer(buffer_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                  VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                  VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                  &staging_buffer, &staging_buffer_memory);
    u8* data;
    vkMapMemory(device, staging_buffer_memory, 0, buffer_size, 0, (void**) &data);
    copy(arena, data);
    dispose(arena);
    vkUnmapMemory(device, staging_buffer_memory);
    create_buffer(buffer_size,
                  VK_BUFFER_USAGE_TRANSFER_DST_BIT |
                  VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                  VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, buffer, memory);
    copy_buffer(staging_buffer, *buffer, buffer_size);
    vkDestroyBuffer(device, staging_buffer, NULL);
    vkFreeMemory(device, staging_buffer_memory, NULL);
}

void create_index_buffer(VkBuffer* buffer, VkDeviceMemory* memory, Arena* arena) 
{
    VkDeviceSize buffer_size = arena->size;
    VkBuffer staging_buffer;
    VkDeviceMemory staging_buffer_memory;
    create_buffer(buffer_size, 
                  VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                  VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                  VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                  &staging_buffer, &staging_buffer_memory);
    u8* data;
    vkMapMemory(device, staging_buffer_memory, 0, buffer_size, 0, (void**) &data);
    copy(arena, data);
    dispose(arena);
    vkUnmapMemory(device, staging_buffer_memory);
    create_buffer(buffer_size,
            VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, buffer, memory);
    copy_buffer(staging_buffer, *buffer, buffer_size);
    vkDestroyBuffer(device, staging_buffer, NULL);
    vkFreeMemory(device, staging_buffer_memory, NULL);
}

void upload_mesh_data()
{
    create_vertex_buffer(vertex_buffer, vertex_buffer_memory, vertex_arena);
    create_vertex_buffer(vertex_buffer + 1, vertex_buffer_memory + 1, vertex_arena + 1);
    create_index_buffer(index_buffer, index_buffer_memory, index_arena);
    create_index_buffer(index_buffer + 1, index_buffer_memory + 1, index_arena + 1);
}

u32 get_align(u32 size, u32 min_align)
{
    if (min_align > 0) {
        return (size + min_align - 1) & ~(min_align - 1);
    }
    return size;
}

void create_uniform_buffer() 
{
    VkPhysicalDeviceProperties properties{};
    vkGetPhysicalDeviceProperties(physical_device, &properties);
    u32 min_align = properties.limits.minUniformBufferOffsetAlignment;
    non_coherent_atom_size = properties.limits.nonCoherentAtomSize;

    u32 size;
    size = sizeof(GlobalUniform);
    dynamic_align[0] = get_align(size, min_align);
    size = sizeof(MaterialUniform);
    dynamic_align[1] = get_align(size, min_align);
    size = sizeof(ObjectUniform);
    dynamic_align[2] = get_align(size, min_align);
    bone_stride = sizeof(Bone);

    material_offset = dynamic_align[0];
    object_offset = material_offset + dynamic_align[1] * UNIFORM_BUF_MATERIAL;
    bone_offset = object_offset + dynamic_align[2] * UNIFORM_BUF_OBJECT;
    VkDeviceSize buffer_size = bone_offset + bone_stride * UNIFORM_BUF_BONE;
    uniform_buffers.resize(max_frames_in_flight);
    uniform_buffers_memory.resize(max_frames_in_flight);
    uniform_buffers_mapped.resize(max_frames_in_flight);
    for (u32 i = 0; i < max_frames_in_flight; ++i) {
        create_buffer(buffer_size, 
                      VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
                      &uniform_buffers[i],
                      &uniform_buffers_memory[i]);
        vkMapMemory(device, 
                    uniform_buffers_memory[i], 
                    0, 
                    buffer_size, 
                    0, 
                    &uniform_buffers_mapped[i]);
    }
}

void create_descriptor_pool() 
{
    VkDescriptorPoolSize pool_size{};
    pool_size.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    pool_size.descriptorCount = (u32) max_frames_in_flight;
    VkDescriptorPoolSize pool_size_dynamic{};
    pool_size_dynamic.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
    pool_size_dynamic.descriptorCount = (u32) max_frames_in_flight * 2;
    VkDescriptorPoolSize pool_size_sampler{};
    pool_size_sampler.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    pool_size_sampler.descriptorCount = (u32) max_frames_in_flight;
    VkDescriptorPoolSize sizes[] = {
        pool_size,
        pool_size_dynamic,
        pool_size_sampler,
    };
    VkDescriptorPoolCreateInfo pool_info{};
    pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    pool_info.poolSizeCount = 3;
    pool_info.pPoolSizes = sizes;
    pool_info.maxSets = (u32) max_frames_in_flight * 3;
    if (vkCreateDescriptorPool(device, 
                               &pool_info, 
                               NULL, 
                               &descriptor_pool) != VK_SUCCESS) {
        printf("Failed to create descriptor pool\n");
        exit(1);
    }
}

VkDescriptorBufferInfo create_buffer_info(VkBuffer buffer, u32 offset, u32 size)
{
    VkDescriptorBufferInfo buffer_info{};
    buffer_info.buffer = buffer;
    buffer_info.offset = offset;
    buffer_info.range = size;
    return buffer_info;
}

VkWriteDescriptorSet create_buffer_write(u32 dst_set_id,
                                         VkDescriptorBufferInfo* buffer_info,
                                         VkDescriptorType type)
{
    VkWriteDescriptorSet descriptor_write{};
    descriptor_write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptor_write.dstSet = descriptor_sets[dst_set_id];
    descriptor_write.dstBinding = 0;
    descriptor_write.dstArrayElement = 0;
    descriptor_write.descriptorType = type;
    descriptor_write.descriptorCount = 1;
    descriptor_write.pBufferInfo = buffer_info;
    descriptor_write.pImageInfo = NULL;
    descriptor_write.pTexelBufferView = NULL;
    return descriptor_write;
}

void create_descriptor_sets() 
{
    VkDescriptorSetLayout layouts[] = {
        descriptor_set_layouts[0],
        descriptor_set_layouts[1],
        descriptor_set_layouts[2],
        descriptor_set_layouts[3],
        descriptor_set_layouts[0],
        descriptor_set_layouts[1],
        descriptor_set_layouts[2],
        descriptor_set_layouts[3],
    };
    VkDescriptorSetAllocateInfo alloc_info{};
    alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    alloc_info.descriptorPool = descriptor_pool;
    alloc_info.descriptorSetCount = (u32) max_frames_in_flight * 3;
    alloc_info.pSetLayouts = layouts;
    if (vkAllocateDescriptorSets(device, &alloc_info, 
                                 descriptor_sets) != VK_SUCCESS) {
        printf("Failed to allocate descriptor sets\n");
        exit(1);
    }
    u32 prev_frame = max_frames_in_flight - 1;
    VkDescriptorBufferInfo buffer_info{};
    VkWriteDescriptorSet writes[2];

    for (u32 i = 0; i < max_frames_in_flight; ++i) {
        VkBuffer buffer = uniform_buffers[i];
        buffer_info = create_buffer_info(buffer, 0, sizeof(GlobalUniform));
        VkDescriptorImageInfo image_info{};
        image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        image_info.imageView = render_image_views[prev_frame];
        image_info.sampler = texture_sampler;
        writes[0] = create_buffer_write(0 + i * 3, &buffer_info, 
                                        VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
        writes[1] = VkWriteDescriptorSet{};
        writes[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writes[1].dstSet = descriptor_sets[0 + i * 4];
        writes[1].dstBinding = 1;
        writes[1].dstArrayElement = 0;
        writes[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        writes[1].descriptorCount = 1;
        writes[1].pImageInfo = &image_info;
        vkUpdateDescriptorSets(device, 2, writes, 0, NULL);

        buffer_info = create_buffer_info(buffer, material_offset, sizeof(MaterialUniform));
        writes[0] = create_buffer_write(1 + i * 4, &buffer_info, 
                                        VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC);
        vkUpdateDescriptorSets(device, 1, writes, 0, NULL);

        buffer_info = create_buffer_info(buffer, object_offset, sizeof(ObjectUniform));
        writes[0] = create_buffer_write(2 + i * 4, &buffer_info, 
                                        VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC);
        vkUpdateDescriptorSets(device, 1, writes, 0, NULL);

        buffer_info = create_buffer_info(buffer, bone_offset, bone_stride * UNIFORM_BUF_BONE);
        writes[0] = create_buffer_write(3 + i * 4, &buffer_info, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
        vkUpdateDescriptorSets(device, 1, writes, 0, NULL);

        prev_frame = (prev_frame + 1) % max_frames_in_flight;
    }
}

void cleanup_swapchain() 
{
    vkDestroyImageView(device, depth_image_view, NULL);
    vkDestroyImage(device, depth_image, NULL);
    vkFreeMemory(device, depth_image_memory, NULL);
    for (u32 i = 0; i < max_frames_in_flight; ++i) {
        vkDestroyFramebuffer(device, framebuffers[i], NULL);
        vkDestroyImageView(device, render_image_views[i], NULL);
        vkDestroyImage(device, render_images[i], NULL);
        vkFreeMemory(device, render_image_memory[i], NULL);
    }
    for (VkImageView image_view : swap_chain_image_views) {
        vkDestroyImageView(device, image_view, NULL);
    }
    vkDestroySwapchainKHR(device, swap_chain, NULL);
    for (u32 i = 0; i < PIPELINE_COUNT; ++i ) {
        vkDestroyPipeline(device, graphics_pipelines[i], NULL);
        vkDestroyPipelineLayout(device, pipeline_layouts[i], NULL);
    }
}

void create_depth_resources();

void recreate_swap_chain(GLFWwindow* window) 
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
    create_swap_chain(window);
    create_render_images();
    create_image_views();
    create_depth_resources();
    create_framebuffers();
    create_pipelines();

    vkDestroyDescriptorPool(device, descriptor_pool, NULL);
    create_descriptor_pool();
    create_descriptor_sets();
}

void create_command_buffers() 
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
        exit(1);
    }
}

void create_sync_objects() 
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
            exit(1);
        }
    }
}

bool has_stencil_component(VkFormat format)
{
    return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
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
    VkPipelineStageFlags source_stage{};
    VkPipelineStageFlags destination_stage{};
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
        source_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destination_stage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    } else {
        printf("Unsupported layout transition\n");
        return;
    }
    vkCmdPipelineBarrier(cmd_buffer, source_stage, destination_stage, 0,
                         0, NULL, 0, NULL, 1, &barrier);
    end_single_time_commands(cmd_buffer);
}

void create_depth_resources()
{
    VkFormat depth_format = find_depth_format();
    create_image(swap_chain_extent.width, swap_chain_extent.height,
                 VK_SAMPLE_COUNT_1_BIT, depth_format,
                 VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
                 VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                 &depth_image, &depth_image_memory);
    depth_image_view = create_image_view(depth_image, depth_format, 
                                         VK_IMAGE_ASPECT_DEPTH_BIT);
    transition_image_layout(depth_image, depth_format, 
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
    region.imageExtent = { width, height, 1 };
    vkCmdCopyBufferToImage(cmd_buffer, buffer, image,
                           VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
    end_single_time_commands(cmd_buffer);
}

void create_texture_image()
{
    i32 tex_width;
    i32 tex_height;
    i32 tex_channels;
    char path_buffer[1024];
    const char* file = "assets/texture.jpg";
    strcpy(path_buffer, PATH_PREFIX);
    strcat(path_buffer, file);
    stbi_uc* pixels = stbi_load(path_buffer, &tex_width, &tex_height, &tex_channels, STBI_rgb_alpha);
    VkDeviceSize image_size = tex_width * tex_height * 4;
    if (!pixels) {
        printf("Failed to load texture image: %s\n", path_buffer);
        exit(1);
    }
    VkBuffer staging_buffer;
    VkDeviceMemory staging_buffer_memory;
    create_buffer(image_size, 
                  VK_BUFFER_USAGE_TRANSFER_SRC_BIT, 
                  VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | 
                  VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, 
                  &staging_buffer, &staging_buffer_memory);
    void* data = NULL;
    vkMapMemory(device, staging_buffer_memory, 0, image_size, 0, &data);
    memcpy(data, pixels, image_size);
    vkUnmapMemory(device, staging_buffer_memory);
    stbi_image_free(pixels);
    create_image(tex_width, tex_height, 
                 VK_SAMPLE_COUNT_1_BIT,
                 VK_FORMAT_R8G8B8A8_SRGB, 
                 VK_IMAGE_TILING_OPTIMAL, 
                 VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, 
                 VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 
                 &texture_image, &texture_image_memory);
    transition_image_layout(texture_image, 
                            VK_FORMAT_R8G8B8A8_SRGB, 
                            VK_IMAGE_LAYOUT_UNDEFINED, 
                            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    copy_buffer_to_image(staging_buffer, texture_image, tex_width, tex_height);
    transition_image_layout(texture_image, 
                            VK_FORMAT_R8G8B8A8_SRGB, 
                            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 
                            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    vkDestroyBuffer(device, staging_buffer, NULL);
    vkFreeMemory(device, staging_buffer_memory, NULL);
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
    sampler_info.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    sampler_info.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    sampler_info.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
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
    if (vkCreateSampler(device, &sampler_info, NULL, &texture_sampler) != VK_SUCCESS) {
        printf("Failed to create texture sampler\n");
        exit(1);
    }
}

void init_vulkan(GLFWwindow* window) 
{
    current_frame = 0;

    create_instance();
    create_surface(window);
    pick_physical_device(&physical_device);
    create_logical_device();
    create_swap_chain(window);
    create_render_images();
    create_image_views();
    create_render_pass();
    create_descriptor_set_layouts();
    create_pipelines();
    create_command_pool();
    create_depth_resources();
    create_framebuffers();
    // ENSURE(create_texture_image(), 20);
    // create_texture_image_view();
    create_texture_sampler();
    upload_mesh_data();
    create_uniform_buffer();
    create_descriptor_pool();
    create_descriptor_sets();
    create_command_buffers();
    create_sync_objects();
}

void flush_uniform_buffer()
{
    vkFlushMappedMemoryRanges(device, range_count, ranges);
    range_count = 0;
}

void update_uniform_memory(u8* memory, u32 offset, u32 size, u32 current_frame)
{
    memcpy((u8*) uniform_buffers_mapped[current_frame] + offset, memory, size);
    VkMappedMemoryRange range{};
    range.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
    range.pNext = NULL;
    range.memory = uniform_buffers_memory[current_frame];
    range.offset = offset;
    range.size = get_align(size, non_coherent_atom_size);
    ranges[range_count] = range;
    ++range_count;
}

void update_global_uniform(glm::vec3 camera_pos, glm::mat4 proj_view)
{
    GlobalUniform ubo;
    ubo.camera_pos = camera_pos;
    ubo.proj_view = proj_view;
    ubo.jitter_index = jitter_index;
    ubo.screen_size = glm::vec2(swap_chain_extent.width, swap_chain_extent.height);
    jitter_index = (jitter_index + 1) % 5;
    update_uniform_memory((u8*) &ubo, 0, sizeof(GlobalUniform), current_frame);
}

u32 alloc_object_uniform(glm::mat4* model, glm::mat4* prev_mvp) 
{
    assert(uniform_object_alloc < UNIFORM_BUF_OBJECT);
    u32 slot = uniform_object_alloc++;
    float time = glfwGetTime();
    ObjectUniform ubo;
    ubo.prev_mvp = *prev_mvp;
    ubo.model = *model;
    u32 offset = object_offset + dynamic_align[2] * slot;
    update_uniform_memory((u8*) &ubo, offset, sizeof(ObjectUniform), current_frame);
    return slot;
}

void init_materials()
{
    MaterialUniform water;
    water.smoothness = 1.0;
    water.roughness = 0.10;
    water.specular = glm::vec3(0.02, 0.02, 0.02);
    water.diffuse = glm::vec3(0.07, 0.07, 0.3);

    MaterialUniform gold;
    gold.smoothness = 0.1;
    gold.roughness = 0.20;
    gold.specular = glm::vec3(1.00, 0.78, 0.34);
    gold.diffuse = glm::vec3(0.0, 0.0, 0.0);

    MaterialUniform floor;
    floor.smoothness = 0.05;
    floor.roughness = 1.00;
    floor.specular = glm::vec3(0.01, 0.01, 0.01);
    floor.diffuse = glm::vec3(0.5, 0.5, 0.5);

    for (u32 i = 0; i < max_frames_in_flight; ++i) {
        update_uniform_memory((u8*) &water, 
                              material_offset, 
                              sizeof(MaterialUniform),
                              i);
        update_uniform_memory((u8*) &gold, 
                              material_offset + dynamic_align[1], 
                              sizeof(MaterialUniform),
                              i);
        update_uniform_memory((u8*) &floor, 
                              material_offset + 2 * dynamic_align[1], 
                              sizeof(MaterialUniform),
                              i);
    }
}

void draw_object(glm::mat4* transform, glm::mat4* prev_mvp, Model* model, u32 material)
{
    u32 slot = alloc_object_uniform(transform, prev_mvp);

    Message message;
    message.pipeline = 0;
    message.object.uniform_slot = slot;
    message.object.material = material;
    message.object.vertex_offset = model->vertex_offset;
    message.object.index_count = model->index_count;
    message.object.index_offset = model->index_offset;
    assert(render_queue.message_count < MAX_MESSAGES);
    render_queue.messages[render_queue.message_count++] = message;
}

void draw_rigged(glm::mat4* transform, glm::mat4* prev_mvp, Model* model, u32 material)
{
    u32 slot = alloc_object_uniform(transform, prev_mvp);

    Message message;
    message.pipeline = 1;
    message.object.uniform_slot = slot;
    message.object.material = material;
    message.object.vertex_offset = model->vertex_offset;
    message.object.index_count = model->index_count;
    message.object.index_offset = model->index_offset;
    assert(render_queue.message_count < MAX_MESSAGES);
    render_queue.messages[render_queue.message_count++] = message;
}

void bind_pipeline(VkCommandBuffer buffer, u32 pipeline)
{
    vkCmdBindPipeline(buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphics_pipelines[pipeline]);
    VkBuffer vertex_buffers[] = {vertex_buffer[pipeline]};
    VkDeviceSize offsets[] = {0};
    vkCmdBindVertexBuffers(buffer, 0, 1, vertex_buffers, offsets);
    vkCmdBindIndexBuffer(buffer, index_buffer[pipeline], 0, VK_INDEX_TYPE_UINT32);
    vkCmdBindDescriptorSets(buffer, VK_PIPELINE_BIND_POINT_GRAPHICS,  pipeline_layouts[pipeline], 
                            0, 1, descriptor_sets + 0 + current_frame * 3, 0, NULL);
}

void draw_entry(VkCommandBuffer buffer, Message message)
{
    u32 material = message.object.material;
    u32 uniform_slot = message.object.uniform_slot;
    u32 index_count = message.object.index_count;
    u32 index_offset = message.object.index_offset;
    u32 vertex_offset = message.object.vertex_offset;

    // Start 1
    u32 dynamic_offsets[] = { material * dynamic_align[1], uniform_slot * dynamic_align[2] };
    // TODO: pipeline_layouts[0]?
    vkCmdBindDescriptorSets(buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_layouts[0], 1,
                            2, descriptor_sets + 1 + current_frame * 3, 2, dynamic_offsets);
    vkCmdDrawIndexed(buffer, index_count, 1, index_offset, vertex_offset, 0);
}

void record_command_buffer(VkCommandBuffer buffer, u32 image_index) 
{
    VkCommandBufferBeginInfo begin_info{};
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    begin_info.flags = 0;
    begin_info.pInheritanceInfo = NULL;
    if (vkBeginCommandBuffer(buffer, &begin_info) != VK_SUCCESS) {
        printf("Failed to begin recording command buffer\n");
    }
    VkRenderPassBeginInfo render_pass_info{};
    render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    render_pass_info.renderPass = render_pass;
    render_pass_info.framebuffer = framebuffers[current_frame];
    render_pass_info.renderArea.offset = {0, 0};
    render_pass_info.renderArea.extent = swap_chain_extent;
    VkClearValue clear_values[] = {
        {{0, 0, 0}},                                        // Color buffer
        {1.0f, 0}                                           // Depth buffer
    };
    render_pass_info.clearValueCount = 2;
    render_pass_info.pClearValues = clear_values;

    vkCmdBeginRenderPass(buffer, &render_pass_info, VK_SUBPASS_CONTENTS_INLINE);
    
    for (u32 i = 0; i < render_queue.message_count; ++i) {
        Message message = render_queue.messages[i];
        bind_pipeline(buffer, message.pipeline);
        draw_entry(buffer, message);
    }
    
    vkCmdEndRenderPass(buffer);

    // TODO: clean this up
    // TODO: switch image layout of other render buffer
    VkImageMemoryBarrier swap_chain_to_dst{};
    swap_chain_to_dst.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    swap_chain_to_dst.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    swap_chain_to_dst.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    swap_chain_to_dst.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    swap_chain_to_dst.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    swap_chain_to_dst.image = swap_chain_images[image_index];
    swap_chain_to_dst.subresourceRange.baseMipLevel = 0;
    swap_chain_to_dst.subresourceRange.levelCount = 1;
    swap_chain_to_dst.subresourceRange.baseArrayLayer = 0;
    swap_chain_to_dst.subresourceRange.layerCount = 1;
    swap_chain_to_dst.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    swap_chain_to_dst.srcAccessMask = 0;
    swap_chain_to_dst.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    vkCmdPipelineBarrier(buffer,
                         VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                         VK_PIPELINE_STAGE_TRANSFER_BIT,
                         0,
                         0, NULL,
                         0, NULL,
                         1, &swap_chain_to_dst);

    VkImageSubresourceLayers subresources;
    subresources.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    subresources.mipLevel = 0;
    subresources.baseArrayLayer = 0;
    subresources.layerCount = 1;
    VkImageBlit blit{};
    blit.srcOffsets[0] = {0, 0, 0};
    blit.srcOffsets[1].x = (i32) swap_chain_extent.width;
    blit.srcOffsets[1].y = (i32) swap_chain_extent.height;
    blit.srcOffsets[1].z = 1;
    blit.dstOffsets[0] = {0, 0, 0};
    blit.dstOffsets[1].x = (i32) swap_chain_extent.width;
    blit.dstOffsets[1].y = (i32) swap_chain_extent.height;
    blit.dstOffsets[1].z = 1;
    blit.srcSubresource = subresources;
    blit.dstSubresource = subresources;
    vkCmdBlitImage(buffer,
                   render_images[current_frame],
                   VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                   swap_chain_images[image_index],
                   VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                   1,
                   &blit,
                   VK_FILTER_NEAREST);

    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = render_images[current_frame];
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    VkImageMemoryBarrier swap_barrier{};
    swap_barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    swap_barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    swap_barrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    swap_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    swap_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    swap_barrier.image = swap_chain_images[image_index];
    swap_barrier.subresourceRange.baseMipLevel = 0;
    swap_barrier.subresourceRange.levelCount = 1;
    swap_barrier.subresourceRange.baseArrayLayer = 0;
    swap_barrier.subresourceRange.layerCount = 1;
    swap_barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    swap_barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    swap_barrier.dstAccessMask = 0;
    VkImageMemoryBarrier barriers[] = {
        barrier,
        swap_barrier
    };
    vkCmdPipelineBarrier(buffer, 
                         VK_PIPELINE_STAGE_TRANSFER_BIT,
                         VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                         0,
                         0, NULL,
                         0, NULL,
                         2, barriers);

    if (vkEndCommandBuffer(buffer) != VK_SUCCESS) {
        printf("Failed to record command buffer\n");
        return;
    }
    return;
}

void start_frame()
{
    uniform_object_alloc = 0;
    render_queue.message_count = 0;
}

void end_frame(GLFWwindow* window) 
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
        recreate_swap_chain(window);
        return;
    } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
        printf("Failed to acquire swap chain image\n");
        return;
    }
    vkResetFences(device, 1, &in_flight_fences[current_frame]);
    vkResetCommandBuffer(command_buffers[current_frame], 0);

    flush_uniform_buffer();
    
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
        recreate_swap_chain(window);
    } else if (result != VK_SUCCESS) {
        printf("Failed to present swap chain image\n");
        return;
    }

    current_frame = (current_frame + 1) % max_frames_in_flight;
}

void cleanup_vulkan() 
{
    vkDeviceWaitIdle(device);

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
    vkDestroyDescriptorSetLayout(device, descriptor_set_layouts[0], NULL);
    vkDestroyDescriptorSetLayout(device, descriptor_set_layouts[1], NULL);
    vkDestroyDescriptorSetLayout(device, descriptor_set_layouts[2], NULL);
    for (u32 i = 0; i < PIPELINE_COUNT; ++i) {
        vkDestroyBuffer(device, vertex_buffer[i], NULL);
        vkFreeMemory(device, vertex_buffer_memory[i], NULL);
        vkDestroyBuffer(device, index_buffer[i], NULL);
        vkFreeMemory(device, index_buffer_memory[i], NULL);
    }
    for (u32 i = 0; i < max_frames_in_flight; ++i) {
        vkDestroySemaphore(device, image_available_semaphores[i], NULL);
        vkDestroySemaphore(device, render_finished_semaphores[i], NULL);
        vkDestroyFence(device, in_flight_fences[i], NULL);
    }
    vkDestroyCommandPool(device, command_pool, NULL);
    vkDestroyRenderPass(device, render_pass, NULL);
    vkDestroyDevice(device, NULL);
    vkDestroySurfaceKHR(instance, surface, NULL);
    vkDestroyInstance(instance, NULL);
}

