#include "utils.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <vulkan/vulkan_core.h>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "init.h"

void drawFrame(State* state);
uint64_t frameCount = 0;

int main(void)
{ 
    State state = {
        .allocator = NULL
    };


    init(&state);
    


    while (!glfwWindowShouldClose(state.window))
    {
        // Proccess all pending events
        glfwPollEvents();
        drawFrame(&state);
        printf("%lu", frameCount);
    }

    vkDeviceWaitIdle(state.device);

    cleanUp(&state);

    exit(EXIT_SUCCESS);
}


void drawFrame(State* state)
{
    // overview
    // wait for previous frame to finish
    // acquire image from swapchain
    // Record command buffer which draws the scene on the acquired image
    // submit the recorded command buffer
    // present swapchain image

    vkWaitForFences(state->device, 1, &state->syncFenInFlight[0], VK_TRUE, UINT64_MAX);
    vkResetFences(state->device, 1, &state->syncFenInFlight[0]);

    // refers to vkImage in swapchian images array (state.swapchainImages)
    uint32_t imageIndex;
    vkAcquireNextImageKHR(state->device, state->swapchain, UINT64_MAX , state->syncSemImgAvail[0], NULL, &imageIndex);

    vkResetCommandBuffer(state->commandBuffer, 0);
    recordCommandBuffer(state->commandBuffer, imageIndex, state);
    
    VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    // submit info
    VkSubmitInfo sbmtInf = {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .pNext = NULL,

        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &state->syncSemImgAvail[0],
        .pWaitDstStageMask = waitStages,

        .commandBufferCount = 1,
        .pCommandBuffers = &state->commandBuffer,

        .signalSemaphoreCount = 1,
        .pSignalSemaphores = &state->syncSemRndrFinsh[0],
    };

    vkQueueSubmit(state->graphicsQueue, 1, &sbmtInf, state->syncFenInFlight[0]);

    VkPresentInfoKHR presentInf = {
        .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .pNext = NULL,

        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &state->syncSemRndrFinsh[0],

        .swapchainCount = 1,
        .pSwapchains = &state->swapchain,
        .pImageIndices = &imageIndex,
        
        .pResults = NULL,
    };
    // graphics queue should be present queue but graphics queue in this case also supports presenting
    vkQueuePresentKHR(state->graphicsQueue, &presentInf);

    frameCount++;
}