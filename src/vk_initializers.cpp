#include "include/vk_initializers.h"

VkCommandPoolCreateInfo cmd_pool_info(u32 queue_family, 
                                      VkCommandPoolCreateFlags flags)
{
    VkCommandPoolCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    info.pNext = NULL;
    info.queueFamilyIndex = queue_family;
    info.flags = flags;
    return info;
}

VkCommandBufferAllocateInfo cmd_buffer_info(VkCommandPool pool, 
                                            u32 count, 
                                            VkCommandBufferLevel level)
{
    VkCommandBufferAllocateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    info.pNext = NULL;
    info.commandPool = pool;
    info.commandBufferCount = count;
    info.level = level;
    return info;
}
