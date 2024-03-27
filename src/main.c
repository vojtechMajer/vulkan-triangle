/**
 * @file main.c
 * @brief Vulkan tutorial implementation in C (https://vulkan-tutorial.com/)
 * @version 0.1
 * @date 2024-03-24
 * 
 */

#include "utils.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <vulkan/vulkan_core.h>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "init.h"

void drawFrame(State* state);
uint32_t currentFrame = 0;

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
        currentFrame = (currentFrame+1) % MAX_FRAMES_IN_FLIGHT; 

    }

    vkDeviceWaitIdle(state.device);

    cleanUp(&state);

    exit(EXIT_SUCCESS);
}


void drawFrame(State* state)
{
    // overview of draw frame function
    // wait for previous frame to finish
    // acquire image from swapchain
    // Record command buffer which draws the scene on the acquired image
    // submit the recorded command buffer
    // present swapchain image

    vkWaitForFences(state->device, 1, state->syncFenInFlight + currentFrame, VK_TRUE, UINT64_MAX);
    vkResetFences(state->device, 1, state->syncFenInFlight + currentFrame );

    // refers to vkImage in swapchian images array (state.swapchainImages)
    uint32_t imageIndex;
    vkAcquireNextImageKHR(state->device, state->swapchain, UINT64_MAX , state->syncSemImgAvail[currentFrame], NULL, &imageIndex);

    vkResetCommandBuffer(state->commandBuffers[currentFrame], 0);
    recordCommandBuffer(state->commandBuffers[currentFrame], imageIndex, state);
    
    VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    // submit info
    // waits on image available
    // signals render finished
    VkSubmitInfo sbmtInf = {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .pNext = NULL,

        .waitSemaphoreCount = 1,
        .pWaitSemaphores = state->syncSemImgAvail + currentFrame,
        .pWaitDstStageMask = waitStages,

        .commandBufferCount = 1,
        .pCommandBuffers = state->commandBuffers + currentFrame,

        .signalSemaphoreCount = 1,
        .pSignalSemaphores = state->syncSemRndrFinsh + currentFrame,
    };

    // signals fence in flight
    vkQueueSubmit(state->graphicsQueue, 1, &sbmtInf, state->syncFenInFlight[currentFrame] );

    // waits on : image rendered
    // signals: nothing
    VkPresentInfoKHR presentInf = {
        .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .pNext = NULL,

        .waitSemaphoreCount = 1,
        .pWaitSemaphores = state->syncSemRndrFinsh + currentFrame,

        .swapchainCount = 1,
        .pSwapchains = &state->swapchain,
        .pImageIndices = &imageIndex,

        .pResults = NULL,
    };

    // graphics queue should be present queue but graphics queue in this case also supports presenting
    vkQueuePresentKHR(state->graphicsQueue, &presentInf);
}