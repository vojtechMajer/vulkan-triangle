#include "utils.h"
#include "debug.h"
#include "init.h"

#include <stdint.h>
#include <stdio.h>
#include <vulkan/vulkan_core.h>

double clamp(int d, int min, int max)
{
  const int t = d < min ? min : d;
  return t > max ? max : t;
}

void copyBuffer(State* state, VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size)
{
    VkCommandBufferAllocateInfo allocInfo = {0};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = state->commandPool;
    allocInfo.commandBufferCount = 1;

    VkCommandBuffer commandBuffer;
    vkAllocateCommandBuffers(state->device, &allocInfo, &commandBuffer);

    VkCommandBufferBeginInfo beginInfo = {0};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(commandBuffer, &beginInfo);

    VkBufferCopy copyRegion = {0};
    copyRegion.srcOffset = 0; // Optional
    copyRegion.dstOffset = 0; // Optional
    copyRegion.size = size;

    vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

    vkEndCommandBuffer(commandBuffer);

    VkSubmitInfo submitInfo = {0};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    vkQueueSubmit(state->graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(state->graphicsQueue);

    vkFreeCommandBuffers(state->device, state->commandPool, 1, &commandBuffer);

}

void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex, State* state)
{
    VkCommandBufferBeginInfo beginInf = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .pNext = NULL,
        .flags = 0,

        .pInheritanceInfo = NULL
    };

    assertVk( vkBeginCommandBuffer(commandBuffer, &beginInf),
    "failed to begin recording command buffer", "began command buffer recording");

    VkClearValue clearValue = {{{0,0,0}}};

    VkRenderPassBeginInfo renderPassBeginInf = {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,

        .renderPass = state->renderPass,
        .framebuffer = state->swapChainFrameBuffers[imageIndex],

        .renderArea.offset = {0,0},
        .renderArea.extent = state->extent,
        // clear color
        .clearValueCount = 1,
        .pClearValues = &clearValue
    };
    vkCmdBeginRenderPass(commandBuffer, &renderPassBeginInf, VK_SUBPASS_CONTENTS_INLINE);

    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, state->graphicsPipeline);
    
    VkDeviceSize offsets[] = {0}; 

    vkCmdBindVertexBuffers(commandBuffer, 0, 1, &state->vertexBuffer, offsets);

    vkCmdBindIndexBuffer(commandBuffer, state->indexBuffer, 0, VK_INDEX_TYPE_UINT16);

    // Set dynamic stages
    VkViewport viewport = { 
        
        .x = 0,
        .y = 0,

        .width = (float) state->extent.width,
        .height = (float) state->extent.height,

        .minDepth = 0,
        .maxDepth = 1,
    };

    vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
   
    VkRect2D scissor = {
        .offset = {0,0},
        .extent = state->extent
    };

    vkCmdSetScissor(commandBuffer, 0, 1, &scissor);


    // vkCmdDraw(commandBuffer, 3, 1, 0, 0);
    vkCmdDrawIndexed(commandBuffer, 6, 1, 0, 0, 0);

    vkCmdEndRenderPass(commandBuffer);

    assertVk(vkEndCommandBuffer(commandBuffer), "Failed to record command buffer", "recorded command buffer");
    

}

void createBuffer(State* state, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer* buffer, VkDeviceMemory* bufferMemory)
{
    VkBufferCreateInfo crtInf = {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .pNext = NULL,
        .flags = 0,

        .size = size,
        .usage = usage,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,

    };

    assertVk(vkCreateBuffer(state->device, &crtInf, state->allocator, buffer), "failed to Created buffer", "Created buffer");

    VkMemoryRequirements memReq;

    vkGetBufferMemoryRequirements(state->device, *buffer, &memReq);

    VkMemoryAllocateInfo allocInf = {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .pNext = NULL,
        .allocationSize = memReq.size,
        .memoryTypeIndex = findMemoryType(state, memReq.memoryTypeBits, properties),
    };

    assertVk(vkAllocateMemory(state->device, &allocInf, state->allocator, bufferMemory), "failed to allocate memory", "allocate memory");

    vkBindBufferMemory(state->device, *buffer,* bufferMemory, 0);
}


void recreateSwapchain(State* state)
{

    vkDeviceWaitIdle(state->device);
    cleanUpSwapchain(state);

    createSwapchain(state);
    retrieveSwapchainImages(state);
    createImageViews(state);
    createFramebuffers(state);
}