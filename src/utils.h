#ifndef __UTILS_H__
#define __UTILS_H__

#include "init.h"
#include <cglm/types.h>
#include <vulkan/vulkan_core.h>

// vertex.h
typedef struct Vertex {
    vec2 pos;
    vec3 color;
}Vertex;


// whatever just fill the hole hole filler
double clamp(int d, int min, int max);
void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex, State* state);
void recreateSwapchain(State* state);

#endif // __UTILS_H__