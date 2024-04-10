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


    vkCmdDraw(commandBuffer, 3, 1, 0, 0);

    vkCmdEndRenderPass(commandBuffer);

    assertVk(vkEndCommandBuffer(commandBuffer), "Failed to record command buffer", "recorded command buffer");
    

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