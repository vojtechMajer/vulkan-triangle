#include "init.h"

#include "debug.h"
#include <GLFW/glfw3.h>
#include <stdint.h>
#include <stdlib.h>
#include <vulkan/vulkan_core.h>

#include "utils.h"

void init(State* state)
{
    // Init GLFW
    assert(glfwInit(), "failed to intialize glfw", "initialized glfw");

    // Create Window
    createWindow(state);
    
    // Init vulkan instance
    assertVk(initVulkan(state, &state->instance), "failed to create instance", "Created instance");

    // Create Surface
    VkResult rslt = glfwCreateWindowSurface(state->instance, state->window, state->allocator, &state->surface);
    assertVk(rslt, "Failed to create window surface", "Created window surface");

    // Select Physical Device
    assertVk( selectPhysicalDevice(state, &state->physicalDevice), "Failed to select physical device", "Selected physical device" );

    // Pick queue family index 
    pickQueueFamily(state);

    // Create Logical Device
    createLogicalDevice(state);

    // Get Graphics queue
    vkGetDeviceQueue(state->device, state->queueFamilyIndex, 0, &state->graphicsQueue);

    // Create swapchain
    // Terms: 
    // swapchain -> list of image buffers that are displayed to the user
    // image view -> additional information attached to the resource describing how the resource will be used

    // Needs: 
    // VK_KHR_swapchain Instance extension (VK_KHR_SWAPCHAIN_EXTENSION_NAME)
    // VK_KHR_surface Instance extension (VK_KHR_SURFACE_EXTENSION_NAME)
    // VK_KHR_swapchain Device extension (VK_KHR_SWAPCHAIN_EXTENSION_NAME)
    
    // set requested Image count will be overridden if invalid 
    state->swapchainImageCount = REQUESTED_SWAPCHAIN_IMAGE_COUNT;
    // set default extent 
    state->extent.width = WINDOW_WIDTH;
    state->extent.height = WINDOW_HEIGHT;

    createSwapchain(state);
    // retrieve swapchain images for vkImageViews
    retrieveSwapchainImages(state);
    createImageViews(state);
    
    createRenderPass(state);
    createGraphicsPipeline(state);

    createFramebuffers(state);

    createCommandPool(state);

    allocateCommandBuffer(state);

    createSyncObject(state);    
}

void createWindow(State* state)
{
    // Create Window
    // Set Window Hints
    // window will be created for Vulkan not OpenGL
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    // no window resizing
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    state->window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Vulkan Triangle", NULL, NULL);
    assert(state->window, "failed to create window", "Created Window");

}

VkResult initVulkan(State* state, VkInstance* pInstance)
{

    VkApplicationInfo appInf = {
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,

        // specifies what version of vulkan will be used
        .apiVersion = VK_API_VERSION_1_3,

        .pApplicationName = "Vulkan Triangle",
        .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
        
        .pEngineName = "Real engine",
        .engineVersion = VK_MAKE_VERSION(1, 0, 0),

    };

    // Loads all instance extensions required by glfw

    // glfw extension count
    uint32_t glfwExtCnt;

    const char** requiredExtensions = glfwGetRequiredInstanceExtensions(&glfwExtCnt); 

    VkInstanceCreateInfo crtInfo = {

        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pNext = NULL,
        .flags = 0,

        .pApplicationInfo = &appInf,
        
        // no validation layers are explicitly used, use vkConfig utility instead 
        .enabledLayerCount = 0,
        .ppEnabledLayerNames = NULL,
        
        .enabledExtensionCount = glfwExtCnt,
        .ppEnabledExtensionNames = requiredExtensions,
    };

    return vkCreateInstance(&crtInfo, state->allocator, pInstance); 

}

VkResult selectPhysicalDevice(State* state, VkPhysicalDevice* physycalDevice)
{
    uint32_t physDevCount;
    VkPhysicalDevice* physDevs;
    
    VkResult rslt;
    vkEnumeratePhysicalDevices(state->instance, &physDevCount, NULL);
    assert(physDevCount, "No device found", "Enumerated devices");
    physDevs = (VkPhysicalDevice*) malloc(sizeof(VkPhysicalDevice) * physDevCount);
    rslt = vkEnumeratePhysicalDevices(state->instance, &physDevCount, physDevs);

    // Select Physical device 
    *physycalDevice = physDevs[0];

    free(physDevs);
    physDevs = NULL;

    return rslt;
}

void pickQueueFamily(State* state)
{
    uint32_t propCount;
    VkQueueFamilyProperties* props;

    vkGetPhysicalDeviceQueueFamilyProperties(state->physicalDevice, &propCount, NULL);
    props = (VkQueueFamilyProperties*) malloc(sizeof(VkQueueFamilyProperties) * propCount);
    vkGetPhysicalDeviceQueueFamilyProperties(state->physicalDevice, &propCount, props);

    assert(props,"failed to get queue families properties" , "queried queue family properties");

    // picks first queue with grpahics bit
    for (int i = 0; i<propCount; i++)
    {
        if (props[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
        {
            LOG("Selected queue family with index %d", i);
            state->queueFamilyIndex = i;
            break;
        }
    }

    free(props);
    props = NULL;
}

void createLogicalDevice(State* state)
{
    float quePriority = 1;
    VkDeviceQueueCreateInfo queCrtInfo = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
        .pNext = NULL,
        .flags = 0, 
        .queueFamilyIndex = state->queueFamilyIndex,
        .queueCount = 1,
        .pQueuePriorities = &quePriority,
    };

    VkPhysicalDeviceFeatures deviceFeatures = {0};


    VkDeviceCreateInfo crtInf  = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO, 
        .pNext = NULL,

        .flags = 0,

        .queueCreateInfoCount = 1, 
        .pQueueCreateInfos = &queCrtInfo, 

        .pEnabledFeatures = &deviceFeatures,
        // Extensions
        .enabledExtensionCount = 1,
        .ppEnabledExtensionNames = (const char *[] ){VK_KHR_SWAPCHAIN_EXTENSION_NAME},
        
        // Deprecated
        .enabledLayerCount = 0,
        .ppEnabledLayerNames = NULL

    };

    assertVk( vkCreateDevice(state->physicalDevice, &crtInf, state->allocator, &state->device),"Failed to create logical device", "Created logical device" );

}

void createSwapchain(State* state)
{
    VkSurfaceCapabilitiesKHR surfCaps;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(state->physicalDevice, state->surface, &surfCaps);

    // if swapchain image count is invalid override it to minimal image count
    state->swapchainImageCount = (state->swapchainImageCount > surfCaps.maxImageCount || state->swapchainImageCount < surfCaps.minImageCount )? surfCaps.minImageCount : state->swapchainImageCount ;

    state->swapchainFormat = selectSwapchainFormat(state);

    state->extent.width = clamp(state->extent.width, surfCaps.minImageExtent.width, surfCaps.maxImageExtent.width); 
    state->extent.height = clamp(state->extent.height, surfCaps.minImageExtent.height, surfCaps.maxImageExtent.height); 
    

    VkSwapchainCreateInfoKHR swpchnCrtInf = {
        .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .pNext = NULL,
        .flags = 0,
        
        .surface = state->surface,
        .minImageCount = state->swapchainImageCount,
        
        .imageFormat = state->swapchainFormat.format,
        .imageColorSpace = state->swapchainFormat.colorSpace,

        .imageExtent = state->extent,
        
        // for for non sterescopic 3d app this will be 1
        .imageArrayLayers = 1,
        
        // imageUsage specifies how will be the image used 
        // image can be used to create a VkImageView suitable for use as a color or resolve attachment in a VkFramebuffer.
        .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        
        // decides if more queue families will have access to this image
        .imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
        
        // since imageSharingmode is exclusive this must be 1  
        .queueFamilyIndexCount = 1,
        .pQueueFamilyIndices = &state->queueFamilyIndex,

        .preTransform = surfCaps.currentTransform,

        .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,

        .presentMode = VK_PRESENT_MODE_FIFO_KHR,
        
        .clipped = VK_TRUE,

        .oldSwapchain = NULL

    };

    assertVk(
    vkCreateSwapchainKHR(state->device, &swpchnCrtInf, state->allocator, &state->swapchain)
    , "failed to create swapchain", "created swapchain");

}

void retrieveSwapchainImages(State* state)
{
    vkGetSwapchainImagesKHR(state->device, state->swapchain, &state->swapchainImageCount, NULL);
    state->swapchainImages = malloc(sizeof(VkImage)*state->swapchainImageCount);

    assertVk(vkGetSwapchainImagesKHR(state->device, state->swapchain, &state->swapchainImageCount, state->swapchainImages)
    , "failed to retrieve swapchain images", "retrieved swapchain images");

    LOG("retrieved %u Images", state->swapchainImageCount);
}

void createImageViews(State* state)
{
    state->imageViews = malloc(sizeof(VkImageView)* state->swapchainImageCount);

    for (uint32_t i = 0; i < state->swapchainImageCount; i++)
    {
        VkImageViewCreateInfo crtInf = {
            .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .pNext = NULL,
            .flags = 0,

            .image = state->swapchainImages[i],
            .viewType = VK_IMAGE_VIEW_TYPE_2D,

            .format = state->swapchainFormat.format,
            .components.a = VK_COMPONENT_SWIZZLE_IDENTITY,
            .components.r = VK_COMPONENT_SWIZZLE_IDENTITY,
            .components.g = VK_COMPONENT_SWIZZLE_IDENTITY,
            .components.b = VK_COMPONENT_SWIZZLE_IDENTITY,

            .subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .subresourceRange.baseArrayLayer = 0,
            .subresourceRange.baseMipLevel = 0,
            .subresourceRange.layerCount = 1,
            .subresourceRange.levelCount = 1,
        };

        assertVk(vkCreateImageView(state->device, &crtInf, state->allocator, state->imageViews+i), 
        "Failed to create image view", "created image view");
    }

}

VkSurfaceFormatKHR selectSwapchainFormat(State* state)
{
    uint32_t surfaceFormatCount;
    VkSurfaceFormatKHR* formats;

    vkGetPhysicalDeviceSurfaceFormatsKHR(state->physicalDevice, state->surface, &surfaceFormatCount, NULL);
    formats = (VkSurfaceFormatKHR*) malloc(sizeof(VkSurfaceFormatKHR) * surfaceFormatCount);
    vkGetPhysicalDeviceSurfaceFormatsKHR(state->physicalDevice, state->surface, &surfaceFormatCount, formats);

    assert(formats,"failed to get formats" , "loaded formats");

    
    for (uint32_t i = 0; i < surfaceFormatCount; i++)
    {
        if ((formats[i].format == VK_FORMAT_B8G8R8A8_SRGB || formats[i].format == VK_FORMAT_R8G8B8A8_SRGB) && formats[i].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
        {
            return formats[i];
            LOG("found surface format with RGB8/sRGB format");
            break;
        }
    }

    return  formats[0];

    free(formats);
    formats = NULL;

}

void createRenderPass(State* state)
{
    VkAttachmentDescription colorAttachment = {
        .flags = 0,
        .format = state->swapchainFormat.format,
        .samples = VK_SAMPLE_COUNT_1_BIT,

        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,

        .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,

        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
    };

    VkAttachmentReference colorAttachmentReference = {
        .attachment = 0,
        .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
    };
    VkSubpassDependency dependency = {
        .srcSubpass = VK_SUBPASS_EXTERNAL,
        .dstSubpass = 0,
    
        .srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        .srcAccessMask = 0,

        .dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,

    };
    
    VkSubpassDescription subpass = {
        .flags = 0,

        .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
        // The index of the attachment in this array is directly referenced from the fragment shader with the
        // layout(location = 0) out vec4 outColor
        .colorAttachmentCount = 1,
        .pColorAttachments = &colorAttachmentReference,
    };

    VkRenderPassCreateInfo renderPassCrtInf = {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
        .pNext = NULL,
        .flags =0,

        .attachmentCount = 1,
        .pAttachments = &colorAttachment,
        
        .subpassCount = 1,
        .pSubpasses = &subpass,

        .dependencyCount = 1,
        .pDependencies = &dependency,

    };

    assertVk(vkCreateRenderPass(state->device, &renderPassCrtInf, state->allocator, &state->renderPass),
    "Failed to create render pass", "created render pass");
    


}

void createGraphicsPipeline(State* state)
{
    // TODO: 
    // Shader stages ✓
    // Vertex input -> specifies vertex input attribute and binding ✓
    // Input assembly ✓
    // Tesselation skipping
    // Viewport ✓
    // Rasterization ✓
    // Multi smaple ✓
    // Depth Stencil skipping
    // Color Blending ✓
    // dynamic states ✓

    // Shader stages: the shader modules that define the functionality of the programmable stages of the graphics pipeline
    // Fixed-function state: all of the structures that define the fixed-function stages of the pipeline, like input assembly, rasterizer, viewport and color blending
    // Pipeline layout: the uniform and push values referenced by the shader that can be updated at draw time
    // Render pass: the attachments(buffers) referenced by the pipeline stages and their usage


    // Dynamic states 
    VkDynamicState dynamicStates[] = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};

    VkPipelineDynamicStateCreateInfo dynamicState = {

        .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
        .flags = 0,

        .dynamicStateCount = 2,
        .pDynamicStates = dynamicStates,
    };

    // Shader Stages
    VkShaderModule shaderModules[2];
    // vertex shader
    shaderModules[0] = createShaderModule("shaders/main.vert.spv", state->device, state->allocator);
    // fragment shader
    shaderModules[1] = createShaderModule("shaders/main.frag.spv", state->device, state->allocator);
    
    VkPipelineShaderStageCreateInfo vertShaderStageInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .stage = VK_SHADER_STAGE_VERTEX_BIT,

        .module = shaderModules[0], // vertex shader module
        .pName = "main",
    };

    VkPipelineShaderStageCreateInfo fragmentShaderStageInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .stage = VK_SHADER_STAGE_FRAGMENT_BIT,

        .module = shaderModules[1], // vertex shader module
        .pName = "main",
    };

    VkPipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragmentShaderStageInfo};

    // Vertex input state
    VkPipelineVertexInputStateCreateInfo vertexInputCrtInf = {

        .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        .pNext = NULL,
        .flags = 0,
        
        .vertexBindingDescriptionCount = 0,
        .pVertexBindingDescriptions = NULL,

        .vertexAttributeDescriptionCount = 0,
        .pVertexAttributeDescriptions = NULL,
    };

    // Assembly state
    VkPipelineInputAssemblyStateCreateInfo inputAssemblyCrtInf = {

        .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
        .pNext = NULL,
        .flags = 0,

        .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
        .primitiveRestartEnable = VK_FALSE
    };

    // Viewport    
    VkPipelineViewportStateCreateInfo viewportCrtInf = {

        .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
        .pNext = NULL,
        .flags = 0,

        .viewportCount = 1,
        .scissorCount = 1,
    };

    // Rasterization
    VkPipelineRasterizationStateCreateInfo rasterizerCrtInf = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
        .pNext = NULL,
        .flags = 0,

        .depthClampEnable = VK_FALSE,
        .rasterizerDiscardEnable = VK_FALSE,

        .polygonMode = VK_POLYGON_MODE_FILL,

        .lineWidth = (float)1, // musn't be larger than 1 if wideLines aren't enabled

        .cullMode = VK_CULL_MODE_BACK_BIT,
        .frontFace = VK_FRONT_FACE_CLOCKWISE,
        
        .depthBiasEnable = VK_FALSE,
        .depthBiasClamp = (float)0,
        .depthBiasConstantFactor = (float)0,
        .depthBiasSlopeFactor = (float)0
    };

    // Multisampling
    VkPipelineMultisampleStateCreateInfo multisampleCrtInf = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        .pNext = NULL,
        .flags = 0,
        .sampleShadingEnable = VK_FALSE,
        .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
        .minSampleShading = (float)1,
        .pSampleMask = NULL,
        .alphaToCoverageEnable = VK_FALSE,
        .alphaToOneEnable = VK_FALSE,
    };

    // Depth & Stencil testing

    // Color blending 
    VkPipelineColorBlendAttachmentState colorBlendAttachment = {
        .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
        
        .blendEnable = VK_FALSE,

        .srcColorBlendFactor = VK_BLEND_FACTOR_ONE,
        .dstColorBlendFactor = VK_BLEND_FACTOR_ZERO,

        .colorBlendOp = VK_BLEND_OP_ADD,
        .srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
        .dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
        .alphaBlendOp = VK_BLEND_OP_ADD,
    };

    VkPipelineColorBlendStateCreateInfo colorBlending = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        .logicOpEnable = VK_FALSE,
        .logicOp = VK_LOGIC_OP_COPY,
        .attachmentCount = 1,
        .pAttachments = &colorBlendAttachment,
        .blendConstants[0] = 0.0f,
        .blendConstants[1] = 0.0f,
        .blendConstants[2] = 0.0f,
        .blendConstants[3] = 0.0f,
    };

    // Pipeline layout -> specify uniforms here
    VkPipelineLayoutCreateInfo pipelineLayoutCrtInf = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .pNext = NULL,
        .flags = 0,

        .setLayoutCount = 0,
        .pSetLayouts = NULL,
        
        .pushConstantRangeCount = 0,
        .pPushConstantRanges = NULL
    };

    assertVk(vkCreatePipelineLayout(state->device, &pipelineLayoutCrtInf, state->allocator, &state->pipelineLayout), "failed to Create Pipeline layout", "created pipeline layout");


    VkGraphicsPipelineCreateInfo graphicsPipelineCrtInf = {
        .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .pNext = NULL,
        .flags = 0,

        .stageCount = 2,
        .pStages = shaderStages,

        .pVertexInputState = &vertexInputCrtInf,
        .pInputAssemblyState = &inputAssemblyCrtInf,
        .pTessellationState = NULL, // skipped
        .pViewportState = &viewportCrtInf,
        .pRasterizationState = &rasterizerCrtInf,
        .pMultisampleState = &multisampleCrtInf,
        .pDepthStencilState = NULL,
        .pColorBlendState = &colorBlending,
        .pDynamicState = &dynamicState,

        .layout = state->pipelineLayout,
        .renderPass = state->renderPass,

        .subpass = 0,

        .basePipelineHandle = NULL,
        .basePipelineIndex = -1,
    };

    assertVk(vkCreateGraphicsPipelines(state->device, NULL, 1, &graphicsPipelineCrtInf, state->allocator, &state->graphicsPipeline),
        "failed to create graphcis pipeline", "created graphics pipeline");


    // delete shader modules 
    vkDestroyShaderModule(state->device, shaderModules[0], state->allocator);
    vkDestroyShaderModule(state->device, shaderModules[1], state->allocator);
}

void createFramebuffers(State* state)
{
    state->swapChainFrameBuffers = (VkFramebuffer*) malloc(sizeof(VkFramebuffer)*state->swapchainImageCount);

    for (uint32_t i = 0; i < state->swapchainImageCount; i++)
    {
        VkImageView attachments[] = { state->imageViews[i] };

        VkFramebufferCreateInfo crtInf = {
            .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
            .pNext = NULL,
            .flags = 0,

            .renderPass = state->renderPass,

            .attachmentCount = 1,
            .pAttachments = attachments,

            .width = state->extent.width,
            .height = state->extent.height,
            
            .layers = 1,
        };

        assertVk(vkCreateFramebuffer(state->device, &crtInf, state->allocator, &state->swapChainFrameBuffers[i]), 
        "Failed to create frame buffer", "Created frambuffer");
    };

}

void createCommandPool(State* state)
{
    VkCommandPoolCreateInfo crtInf = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .pNext = NULL,
        .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,

        .queueFamilyIndex = state->queueFamilyIndex
    };

    assertVk(vkCreateCommandPool(state->device, &crtInf, state->allocator, &state->commandPool) ,
        "failed to create command pool", "created command pool");
}

void allocateCommandBuffer(State* state)
{
    VkCommandBufferAllocateInfo allocInf = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .pNext = NULL,
        
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = 1,

        .commandPool = state->commandPool
    };

    assertVk(vkAllocateCommandBuffers(state->device, &allocInf, &state->commandBuffer), 
    "failed to allocate command buffer", "allocated command buffer");
    ;
}





void createSyncObject(State* state)
{
    // semaphore create info
    VkSemaphoreCreateInfo semCrtInf = {
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
        .pNext = NULL,
        .flags = 0
    };

    // fence create info
    VkFenceCreateInfo fenCrtInf = {
        .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
        .pNext = NULL,
        .flags = VK_FENCE_CREATE_SIGNALED_BIT
    };

    state->syncSemImgAvail = (VkSemaphore*) malloc(sizeof(VkSemaphore)*1);
    state->syncSemRndrFinsh = (VkSemaphore*) malloc(sizeof(VkSemaphore)*1);
    state->syncFenInFlight = (VkFence*) malloc(sizeof(VkFence)*1);

    assertVk(vkCreateSemaphore(state->device, &semCrtInf, state->allocator, &state->syncSemImgAvail[0]), "failed to create semaphore", "created semaphore");
    assertVk(vkCreateSemaphore(state->device, &semCrtInf, state->allocator, &state->syncSemRndrFinsh[0]), "failed to create semaphore", "created semaphore");

    assertVk(vkCreateFence(state->device, &fenCrtInf, state->allocator, &state->syncFenInFlight[0]), "failed to create fence", "created fence");

}


void cleanUp(State* state)
{
    vkDestroySemaphore(state->device, state->syncSemImgAvail[0], state->allocator);
    vkDestroySemaphore(state->device, state->syncSemRndrFinsh[0], state->allocator);
    vkDestroyFence(state->device, state->syncFenInFlight[0], state->allocator);
    vkDestroyCommandPool(state->device, state->commandPool, state->allocator);
    for (uint32_t i = 0; i < state->swapchainImageCount; i++)
    {
        vkDestroyFramebuffer(state->device, state->swapChainFrameBuffers[i], state->allocator);
    }

    vkDestroyPipeline(state->device, state->graphicsPipeline, state->allocator);
    vkDestroyPipelineLayout(state->device, state->pipelineLayout, state->allocator);
    vkDestroyRenderPass(state->device, state->renderPass, state->allocator);

    for (uint32_t i = 0; i < state->swapchainImageCount; i++)
    {
        vkDestroyImageView(state->device, state->imageViews[i], state->allocator);

    }

    vkDestroySwapchainKHR(state->device, state->swapchain, state->allocator);
    vkDestroySurfaceKHR(state->instance, state->surface, state->allocator);
    vkDestroyDevice(state->device, state->allocator);
    vkDestroyInstance(state->instance, state->allocator);
    
    glfwDestroyWindow(state->window);
    glfwTerminate();
    
    exit(EXIT_SUCCESS);

}

VkShaderModule createShaderModule(const char* pathToShader, VkDevice device, VkAllocationCallbacks* allocator)
{
    
    VkShaderModule shaderModule;
    unsigned long shaderSize;
    char* byteShaderCode = readShader(pathToShader, &shaderSize);    

    VkShaderModuleCreateInfo createInfo = {
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .codeSize = shaderSize,
        .pCode = (uint32_t*)byteShaderCode
    };


    assertVk(vkCreateShaderModule(device, &createInfo, allocator, &shaderModule), "failed to create shader module", "created shader module");
    
    free(byteShaderCode);

    return shaderModule;

}

char* readShader(const char* filename, unsigned long* fileSize)
{
    FILE* file;
    char* bytes;

    file = fopen(filename, "rb");
    assert(file, "failed to open file check file path", "opened shader code");

    // change position to end of file to get size of file
    fseek(file, 0, SEEK_END);
    *fileSize = ftell(file);
    bytes = (char*) malloc(sizeof(char) * (*fileSize));
    
    // set cursor bacl to start to read data properly
    fseek(file,0, SEEK_SET);    
    unsigned long bytesRead = fread(bytes, sizeof(char), (*fileSize), file);
    assert(bytesRead == (*fileSize), "failed to read all data", "read all data from file");

    LOG("bytes read %lu", bytesRead);

    fclose(file);
    return bytes;
}