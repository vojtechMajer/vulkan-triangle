#ifndef __UTILS_H__
#define __UTILS_H__

#include "init.h"
#include <vulkan/vulkan_core.h>

double clamp(int d, int min, int max);
void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex, State* state);
void recreateSwapchain(State* state);

#endif // __UTILS_H__