#ifndef __INIT_H__
#define __INIT_H__

#include <stdint.h>
#include <vulkan/vulkan_core.h>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define WINDOW_WIDTH 800
#define WINDOW_HEIGHT 600

#define REQUESTED_SWAPCHAIN_IMAGE_COUNT 3

#define MAX_FRAMES_IN_FLIGHT 2

typedef struct State
{
    // allocator -> allocator for vulkan objects 
    VkAllocationCallbacks* allocator;

    // instance -> interface for 
    VkInstance instance;
    GLFWwindow* window;
    VkSurfaceKHR surface;

    VkPhysicalDevice physicalDevice;
    uint32_t queueFamilyIndex;
    VkDevice device;

    uint32_t swapchainImageCount;
    VkExtent2D extent;
    VkSurfaceFormatKHR swapchainFormat;
    VkSwapchainKHR swapchain;

    VkImage* swapchainImages;
    VkImageView* imageViews;
    VkFramebuffer* swapChainFrameBuffers;


    VkQueue graphicsQueue;
        
    VkCommandPool commandPool;
    VkCommandBuffer* commandBuffers;

    VkRenderPass renderPass;
    VkPipelineLayout pipelineLayout;
    VkPipeline graphicsPipeline;

    VkBuffer vertexBuffer;
    VkDeviceMemory vertexBufferMemory;

    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;


    // sync objects

    // semaphore image available -> image from swapchain is available(rendered) [swapchain image count]
    VkSemaphore* syncSemImgAvail;
    // semaphore render -> Rendering of image finished [swapchain image count]
    VkSemaphore* syncSemRndrFinsh;
    // Fence image in flight -> image is in flight [swapchain image count]
    VkFence* syncFenInFlight;

    VkBool32 frameBufferResized;

} State;

void init(State* state);

/**
 * @brief Creates window specify window flags here
 * 
 * @param state 
 */
void createWindow(State* state);

/**
 * @brief creates vulkan instance
 * @details Creates vulkan instance with glfw required instance extensions
 * @param state 
 */
VkResult initVulkan(State* state, VkInstance* pInstance);

/**
 * @brief selects first vulkan capable device
 * Requires:
 *  - valid vulkan instance in state
 * @param state
 */
VkResult selectPhysicalDevice(State* state, VkPhysicalDevice* physycalDevice);


/**
 * @brief sets queue family index in state to queue family with support for at least Graphics Queue
 * Requires:
    - Valid instance in state
    - Valid physical device in state
 * @param state 
 */
 void pickQueueFamily(State* state);

 /**
  * @brief Creates a logical device from physical device and queue family index
  * Requires:
    - Valid instance in state
    - Valid physical device in state
    - Valid queue family index
  * @param state 
  */
void createLogicalDevice(State* state);

/**
 * @brief Creates a swapchain
 * Requires:
    - Valid instance in state
    - Valid physical device in state
    - Valid queue family index

    - Set required extent 
    - Set required image count
 * @param state 
 */
void createSwapchain(State* state);
VkSurfaceFormatKHR selectSwapchainFormat(State* state);
void retrieveSwapchainImages(State* state);
void createImageViews(State* state);

/**
 * @brief selects swapchain format of selected physical device in state
 * 
 * @param state 
 * @return VkSurfaceFormatKHR surface with RGB8/sRGB format or first available 
 */
void createRenderPass(State* state);

void createGraphicsPipeline(State* state);

void createFramebuffers(State* state);

void createCommandPool(State* state);

void createVertexBuffer(State* state);
uint32_t findMemoryType(State* state, uint32_t typeFilter, VkMemoryPropertyFlags properties);

void allocateCommandBuffers(State* state);

void createSyncObject(State* state);


void cleanUp(State* state);
void cleanUpSwapchain(State* state);

void framebufferResizeCallback(GLFWwindow* window, int width, int height);

VkShaderModule createShaderModule(const char* pathToShader, VkDevice device, VkAllocationCallbacks* allocator);
char* readShader(const char* filename, unsigned long* fileSize);


#endif