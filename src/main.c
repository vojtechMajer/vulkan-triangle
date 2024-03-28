/**
 * @file main.c
 * @brief Vulkan tutorial implementation in C (https://vulkan-tutorial.com/)
 * @version 0.1
 * @date 2024-03-24
 * 
 */

#include "debug.h"
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

    // refers to vkImage in swapchian images array (state.swapchainImages)
    // image index
    uint32_t imgIndex;
    VkResult acquireImagerslt = vkAcquireNextImageKHR(state->device, state->swapchain, UINT64_MAX , state->syncSemImgAvail[currentFrame], NULL, &imgIndex);

    if (acquireImagerslt == VK_ERROR_OUT_OF_DATE_KHR) {
        recreateSwapchain(state);
        return;
    } 
    else if (acquireImagerslt != VK_SUCCESS && acquireImagerslt != VK_SUBOPTIMAL_KHR)
    {
        assert_my(-1, "failed to acquire swapchain image", "");
    }

    vkResetFences(state->device, 1, state->syncFenInFlight + currentFrame );

    vkResetCommandBuffer(state->commandBuffers[currentFrame], 0);
    recordCommandBuffer(state->commandBuffers[currentFrame], imgIndex, state);
    
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
        .pImageIndices = &imgIndex,

        .pResults = NULL,
    };

    // graphics queue should be present queue but graphics queue in this case also supports presenting
    VkResult queuePresentRslt = vkQueuePresentKHR(state->graphicsQueue, &presentInf);

    if (queuePresentRslt == VK_ERROR_OUT_OF_DATE_KHR || queuePresentRslt == VK_SUBOPTIMAL_KHR)
    {
        recreateSwapchain(state);
    } 
    else if (queuePresentRslt != VK_SUCCESS)
    {
        assert_my(-1, "failed to present swap chain image", "");
    }

    currentFrame = (currentFrame+1) % MAX_FRAMES_IN_FLIGHT; 

}