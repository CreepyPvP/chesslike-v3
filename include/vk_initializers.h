#include <vulkan/vulkan_core.h>
#include "include/defines.h"

VkCommandPoolCreateInfo cmd_pool_info(u32 queue_family, 
                                      VkCommandPoolCreateFlags flags);

VkCommandBufferAllocateInfo cmd_buffer_info(VkCommandPool pool, 
                                            u32 count, 
                                            VkCommandBufferLevel level);


