#include <SDL2/SDL_video.h>
#include <SDL2/SDL_vulkan.h>

#include <cstddef>
#include <vulkan/vulkan_core.h>

#include <algorithm>
#include <cstring>
#include <iostream>
#include <set>
#include <stdexcept>
#include <limits>
#include <cmath>

#include "types/qfamily_indices.hpp"
#include "types/vertex.hpp"
#include "engine.hpp"

#ifdef NDEBUG
    const bool enableValidationLayers = false;
#else
    const bool enableValidationLayers = true;
#endif

const vector<const char*> validationLayers = {
    "VK_LAYER_KHRONOS_validation"  
};

bool isStrEqual( const char *a, const char *b ) {

    return strcmp( a, b ) == 0;
}

const vector<const char*> deviceExtensions = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

void VulkanEngine::setup( SDL_Window* window ) {

    _safe = true;

    if (enableValidationLayers && !checkValidationLayerSupport()) {

        throw std::runtime_error("Vulkan API required validation layers unavailable");
    }

    _window = window;

    createInstance( window );
    createWindowSurface( window );
    pickPhysicalDevice();
    createLogicalDevice();
    createSwapChain();
    createImageViews();
    createRenderPass();
    createRenderPipeline();
    createFramebuffers();
    createCommandPool();
    createCommandBuffer();
    createVertexBuffer();
    createSyncObjects();
}

bool VulkanEngine::checkValidationLayerSupport() {

    uint propertyCount;

    vkEnumerateInstanceLayerProperties( &propertyCount, nullptr );

    vector<VkLayerProperties> availableLayerProps(propertyCount);
    vkEnumerateInstanceLayerProperties( &propertyCount, availableLayerProps.data());

    for (const char* validLayer : validationLayers) {

        bool layerFound = std::any_of( 
            availableLayerProps.begin(), 
            availableLayerProps.end(), 
            [validLayer] (const auto& layerProps) { return isStrEqual( validLayer, layerProps.layerName ); } );

        if ( !layerFound )
            return false;
    }

    return true;
}

void VulkanEngine::createInstance( SDL_Window* window ) {

    VkApplicationInfo appInfo{
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pNext = NULL,
        .pApplicationName = "open-devil",
        .applicationVersion = 0,
        .pEngineName = "open-devil-engine",
        .engineVersion = 0,
        .apiVersion = VK_API_VERSION_1_0
    };

    // List required SDL extensions for Vulkan
    uint pCount;

    SDL_Vulkan_GetInstanceExtensions( window, &pCount, nullptr );

    vector<const char*> extensionNames(pCount);

    SDL_Vulkan_GetInstanceExtensions( window, &pCount, extensionNames.data() );

    VkInstanceCreateInfo instanceInfo{
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pApplicationInfo = &appInfo,
        .enabledLayerCount = 0,
        .ppEnabledLayerNames = NULL,
        .enabledExtensionCount = pCount,
        .ppEnabledExtensionNames = extensionNames.data()
    };

    if ( enableValidationLayers ) {

        instanceInfo.enabledLayerCount = validationLayers.size();
        instanceInfo.ppEnabledLayerNames = validationLayers.data();
    }

    if ( vkCreateInstance(&instanceInfo, NULL, &_instance) != VK_SUCCESS ) {

        throw std::runtime_error("Failed to create Vulkan instance");
        _safe = false;
    }
}

void VulkanEngine::createWindowSurface( SDL_Window* window ) {

    if ( SDL_Vulkan_CreateSurface( window, _instance, &_surface) == SDL_FALSE ) {

        throw std::runtime_error( "Failed to create Vulkan API window surface" );
    }
}

void VulkanEngine::pickPhysicalDevice() {

    uint deviceCount;

    // Find the number of physical devices
    vkEnumeratePhysicalDevices( _instance, &deviceCount, nullptr );

    if ( deviceCount == 0 ) {

        throw std::runtime_error( "Failed to find GPUs with Vulkan support" );
        _safe = false;
    }

    vector<VkPhysicalDevice> devices(deviceCount);

    vkEnumeratePhysicalDevices( _instance, &deviceCount, devices.data() );

    // Search for a suitable device
    auto it = std::find_if( devices.begin(), 
                             devices.end(), 
                             [this](VkPhysicalDevice dev) { return isSuitableDevice(dev); } );

    if ( it == devices.end() ) {

        throw std::runtime_error( "Failed to find a suitable GPU" );
        _safe = false;
    }

    _physicalDevice = *it;

    // Print device name
    VkPhysicalDeviceProperties props;

    vkGetPhysicalDeviceProperties( _physicalDevice, &props );

    std::cout << "Device picked: (" << props.deviceID << ") " << props.deviceName << std::endl;
}

bool VulkanEngine::isSuitableDevice( VkPhysicalDevice device ) {

    VkPhysicalDeviceProperties props;
    VkPhysicalDeviceFeatures features;

    vkGetPhysicalDeviceProperties( device, &props );
    vkGetPhysicalDeviceFeatures( device, &features );

    QueueFamilyIndices queueFamilyIndices = findQueueFamilies( device, _surface );

    bool areExtensionsSupported = checkDeviceExtensionsSupported( device );
    bool swapChainAdequate = false;

    if ( areExtensionsSupported ) {

        auto swapChainSupport = querySwapChainSupport( device, _surface );
        swapChainAdequate = swapChainSupport.isComplete();
    }

    return props.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU &&
           features.geometryShader &&
           areExtensionsSupported && 
           swapChainAdequate &&
           queueFamilyIndices.isComplete();
}

bool VulkanEngine::checkDeviceExtensionsSupported( VkPhysicalDevice device ) {

    uint extensionsCount;
    vkEnumerateDeviceExtensionProperties( device, nullptr, &extensionsCount, nullptr);

    vector<VkExtensionProperties> availableExtensions( extensionsCount );
    vkEnumerateDeviceExtensionProperties( device, nullptr, &extensionsCount, availableExtensions.data() );

    for(const char *requiredExtension : deviceExtensions) {

        bool extensionFound = std::any_of(
            availableExtensions.begin(),
            availableExtensions.end(),
            [requiredExtension] ( const auto availableExtension ) { return isStrEqual( requiredExtension, availableExtension.extensionName ); }
        );

        if (!extensionFound)
            return false;
    }

    return true;
}

VkSurfaceFormatKHR VulkanEngine::chooseSwapSurfaceFormat( const vector<VkSurfaceFormatKHR>& availableFormats ) {

    for ( const auto& availableFormat : availableFormats) {

        if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {

            return availableFormat;
        }
    }

    return availableFormats[0];
}

VkPresentModeKHR VulkanEngine::chooseSwapPresentMode( const vector<VkPresentModeKHR>& availableModes ) {

    auto it = std::find( availableModes.begin(), availableModes.end(), VK_PRESENT_MODE_MAILBOX_KHR );

    if ( it != availableModes.end() ) {

        return *it;
    }

    // Default fallback mode
    return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D VulkanEngine::chooseSwapExtent( const VkSurfaceCapabilitiesKHR& capabilities) {

    if ( capabilities.currentExtent.width != std::numeric_limits<uint>::max() ) {

        return capabilities.currentExtent;
    }

    int width, height;
    SDL_GL_GetDrawableSize( _window, &width, &height );

    VkExtent2D actualExtent = {
        static_cast<uint>(width),
        static_cast<uint>(height)
    };

    actualExtent.width = std::clamp( actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width );
    actualExtent.height = std::clamp( actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height );

    return actualExtent;
}

void VulkanEngine::createLogicalDevice() {

    QueueFamilyIndices familyIndices = findQueueFamilies( _physicalDevice, _surface );

    vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    std::set<uint> uniqueQueueFamilies = { 
        familyIndices.graphicsFamily.value(), 
        familyIndices.presentFamily.value() };

    float queuePriorities[1] = { 1.0f };

    for (uint queueFamilyIndex : uniqueQueueFamilies ) {

        VkDeviceQueueCreateInfo createInfo {
            .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
            .queueFamilyIndex = queueFamilyIndex,
            .queueCount = 1,
            .pQueuePriorities = queuePriorities
        };

        queueCreateInfos.push_back( createInfo );
    }

    VkPhysicalDeviceFeatures deviceFeatures{};

    VkDeviceCreateInfo deviceCreateInfo {
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .queueCreateInfoCount = static_cast<uint>(queueCreateInfos.size()),
        .pQueueCreateInfos = queueCreateInfos.data(),
        .enabledLayerCount = 0,
        .enabledExtensionCount = static_cast<uint>(deviceExtensions.size()),
        .ppEnabledExtensionNames = deviceExtensions.data(),
        .pEnabledFeatures = &deviceFeatures,
    };

    if ( enableValidationLayers ) {

        deviceCreateInfo.enabledLayerCount = validationLayers.size();
        deviceCreateInfo.ppEnabledLayerNames = validationLayers.data();
    }

    if ( vkCreateDevice( _physicalDevice, &deviceCreateInfo, nullptr, &_device) != VK_SUCCESS ) {

        throw std::runtime_error("Vulkan API: Failed to create logical device");
    }


    vkGetDeviceQueue( _device, familyIndices.graphicsFamily.value(), 0, &_graphicsQueue);
    vkGetDeviceQueue( _device, familyIndices.presentFamily.value(), 0, &_presentQueue);
}

void VulkanEngine::createSwapChain() {

    SwapChainSupportDetails supportDetails = querySwapChainSupport( _physicalDevice, _surface );

    auto surfaceFormat = chooseSwapSurfaceFormat( supportDetails.formats );
    auto presentMode = chooseSwapPresentMode( supportDetails.presentModes );
    auto extent = chooseSwapExtent( supportDetails.capabilities );

    // Find image count between min and max image count possible
    uint imageCount = supportDetails.capabilities.minImageCount + 1;

    if ( supportDetails.capabilities.maxImageCount > 0 && imageCount > supportDetails.capabilities.maxImageCount ) {

        imageCount = supportDetails.capabilities.maxImageCount;
    }

    VkSwapchainCreateInfoKHR createInfo{
        .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .surface = _surface,
        .minImageCount = imageCount,
        .imageFormat = surfaceFormat.format,
        .imageColorSpace = surfaceFormat.colorSpace,
        .imageExtent = extent,
        .imageArrayLayers = 1,
        .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        .preTransform = supportDetails.capabilities.currentTransform,
        .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        .presentMode = presentMode,
        .clipped = VK_TRUE,
        .oldSwapchain = VK_NULL_HANDLE,
    };

    QueueFamilyIndices queueFamilyIndices = findQueueFamilies( _physicalDevice, _surface );
    uint queueFamilyIndicesArray[] = { queueFamilyIndices.graphicsFamily.value(), queueFamilyIndices.presentFamily.value() };

    if ( queueFamilyIndices.graphicsFamily == queueFamilyIndices.presentFamily ) {

        createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        createInfo.queueFamilyIndexCount = 0;
        createInfo.pQueueFamilyIndices = nullptr;
    } else {
    
        createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices = queueFamilyIndicesArray;
    }

    if ( vkCreateSwapchainKHR( _device, &createInfo, nullptr, &_swapchain ) != VK_SUCCESS ) {

        throw std::runtime_error( "Vulkan API: failed to create swap chain" );
    }

    vkGetSwapchainImagesKHR( _device, _swapchain, &imageCount, nullptr );
    _swapchainImages.resize( imageCount );

    vkGetSwapchainImagesKHR( _device, _swapchain, &imageCount, _swapchainImages.data() );

    _swapchainImageFormat = surfaceFormat.format;
    _swapchainExtent = extent;
}

void VulkanEngine::createImageViews() {

    for (auto swapchainImage : _swapchainImages) {

        VkImageViewCreateInfo createInfo {
            .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .image = swapchainImage,
            .viewType = VK_IMAGE_VIEW_TYPE_2D,
            .format = _swapchainImageFormat,

            .components {
                .r = VK_COMPONENT_SWIZZLE_IDENTITY,
                .g = VK_COMPONENT_SWIZZLE_IDENTITY,
                .b = VK_COMPONENT_SWIZZLE_IDENTITY,
                .a = VK_COMPONENT_SWIZZLE_IDENTITY,
            },

            .subresourceRange {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .baseMipLevel = 0,
                .levelCount = 1,
                .baseArrayLayer = 0,
                .layerCount = 1
            }
        };

        VkImageView imageView;

        if ( vkCreateImageView( _device, &createInfo, nullptr, &imageView) != VK_SUCCESS ) {

            throw std::runtime_error( "Could not create image view" );
        }

        _swapchainImageViews.push_back( imageView );
    }
}

void VulkanEngine::createRenderPass() {

    VkAttachmentDescription colorAttachment {
        .format = _swapchainImageFormat,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
    };

    VkAttachmentReference colorAttachmentRef {
        .attachment = 0,
        .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
    };

    VkSubpassDescription subpass {
        .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
        .colorAttachmentCount = 1,
        .pColorAttachments = &colorAttachmentRef
    };

    VkSubpassDependency dependency { 
        .srcSubpass = VK_SUBPASS_EXTERNAL, 
        .dstSubpass = 0,
        .srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        .dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        .srcAccessMask = 0,
        .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
    };

    VkRenderPassCreateInfo createInfo {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
        .attachmentCount = 1,
        .pAttachments = &colorAttachment,
        .subpassCount = 1,
        .pSubpasses = &subpass,
        .dependencyCount = 1,
        .pDependencies = &dependency
    };

    if ( vkCreateRenderPass( _device, &createInfo, nullptr, &_renderPass ) != VK_SUCCESS ) {

        throw std::runtime_error( "Failed to create render pass!" );
    }
}

void VulkanEngine::createRenderPipeline() {

    _mainShader = Shader::loadShader( _device, "shaders/main.vert.spv", "shaders/main.frag.spv");

    VkPipelineShaderStageCreateInfo vertShaderStageInfo {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .stage = VK_SHADER_STAGE_VERTEX_BIT,
        .module = _mainShader.getVertexShaderModule(),
        .pName = "main"
    };

    VkPipelineShaderStageCreateInfo fragShaderStageInfo {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
        .module = _mainShader.getFragmentShaderModule(),
        .pName = "main"
    };

    VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };

    vector<VkDynamicState> dynamicStates = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };

    VkPipelineDynamicStateCreateInfo dynamicState {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
        .dynamicStateCount = static_cast<uint>(dynamicStates.size()),
        .pDynamicStates = dynamicStates.data()
    };

    // Vertex input attribute description
    auto bindingDesc = Vertex::getBindingDescription();
    auto attrsDesc = Vertex::getAttributeDescription();

    VkPipelineVertexInputStateCreateInfo vertexInputInfo {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        .vertexBindingDescriptionCount = 1,
        .pVertexBindingDescriptions = &bindingDesc,
        .vertexAttributeDescriptionCount = attrsDesc.size(),
        .pVertexAttributeDescriptions = attrsDesc.data(),
    };

    VkPipelineInputAssemblyStateCreateInfo inputAssembly {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
        .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
        .primitiveRestartEnable = VK_FALSE,  
    };

    VkViewport viewport {
        .x = 0.f,
        .y = 0.f,
        .width = static_cast<float>(std::round(_swapchainExtent.width)),
        .height = static_cast<float>(std::round(_swapchainExtent.height)),
        .minDepth = 0.f,
        .maxDepth = 1.f
    };

    VkRect2D scissor {
        .offset = {0, 0},
        .extent = _swapchainExtent
    };

    VkPipelineViewportStateCreateInfo viewportState {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
        .viewportCount = 1,
        .pViewports = &viewport,
        .scissorCount = 1,
        .pScissors = &scissor
    };

    VkPipelineRasterizationStateCreateInfo rasterizer{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
        .depthClampEnable = VK_FALSE,
        .rasterizerDiscardEnable = VK_FALSE,
        .polygonMode = VK_POLYGON_MODE_FILL,
        .cullMode = VK_CULL_MODE_BACK_BIT,
        .frontFace = VK_FRONT_FACE_CLOCKWISE,
        .depthBiasEnable = VK_FALSE,
        .depthBiasConstantFactor = 0.f,
        .depthBiasClamp = 0.f,
        .depthBiasSlopeFactor = 0.f,
        .lineWidth = 1.f,
    };

    VkPipelineMultisampleStateCreateInfo multisampling {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
        .sampleShadingEnable = VK_FALSE,
        .minSampleShading = 1.f,
        .pSampleMask = nullptr,
        .alphaToCoverageEnable = VK_FALSE,
        .alphaToOneEnable = VK_FALSE
    };

    VkPipelineColorBlendAttachmentState colorBlendAttachment {
        .blendEnable = VK_FALSE,
        .srcColorBlendFactor = VK_BLEND_FACTOR_ONE,
        .dstColorBlendFactor = VK_BLEND_FACTOR_ZERO,
        .colorBlendOp = VK_BLEND_OP_ADD,
        .srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
        .dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
        .alphaBlendOp = VK_BLEND_OP_ADD,
        .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
    };

    VkPipelineColorBlendStateCreateInfo colorBlending {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        .logicOpEnable = VK_FALSE,
        .attachmentCount = 1,
        .pAttachments = &colorBlendAttachment,
        .blendConstants = { 0.f, 0.f, 0.f, 0.f }
    };

    VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .setLayoutCount = 0,
        .pSetLayouts = NULL,
        .pushConstantRangeCount = 0,
        .pPushConstantRanges = nullptr,
    };

    if ( vkCreatePipelineLayout( _device, &pipelineLayoutCreateInfo, nullptr, &_pipelineLayout) != VK_SUCCESS ) {

        throw std::runtime_error("Unable to create pipeline layout");
    }

    VkGraphicsPipelineCreateInfo pipelineCreateInfo {
        .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .stageCount = 2,
        .pStages = shaderStages,
        .pVertexInputState = &vertexInputInfo,
        .pInputAssemblyState = &inputAssembly,
        .pViewportState = &viewportState,
        .pRasterizationState = &rasterizer,
        .pMultisampleState = &multisampling,
        .pDepthStencilState = nullptr,
        .pColorBlendState = &colorBlending,
        .pDynamicState = &dynamicState,
        .layout = _pipelineLayout,
        .renderPass = _renderPass,
        .subpass = 0,
        .basePipelineHandle = VK_NULL_HANDLE,
        .basePipelineIndex = -1
    };

    if ( vkCreateGraphicsPipelines( _device, VK_NULL_HANDLE, 1, &pipelineCreateInfo, nullptr, &_mainGraphicsPipeline ) != VK_SUCCESS ) {

        throw std::runtime_error("Failed to create graphics pipeline");
    }
}

void VulkanEngine::createFramebuffers() {

    for ( auto imageView : _swapchainImageViews ) {

        VkImageView attachments[] { imageView };

        VkFramebufferCreateInfo createInfo {
            .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
            .renderPass = _renderPass,
            .attachmentCount = 1,
            .pAttachments = attachments,
            .width = _swapchainExtent.width,
            .height = _swapchainExtent.height,
            .layers = 1
        };

        VkFramebuffer framebuffer;

        if ( vkCreateFramebuffer( _device, &createInfo, nullptr, &framebuffer ) != VK_SUCCESS ) {

            throw std::runtime_error("Failed to create framebuffer");
        }

        _swapchainFramebuffers.push_back( framebuffer );
    }
}

void VulkanEngine::createCommandPool() {

    auto queueFamilyIndices = findQueueFamilies( _physicalDevice, _surface );

    VkCommandPoolCreateInfo createInfo {
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
        .queueFamilyIndex = queueFamilyIndices.graphicsFamily.value()
    };

    if ( vkCreateCommandPool( _device, &createInfo, nullptr, &_commandPool) != VK_SUCCESS ) {

        throw std::runtime_error("Failed to create command pool");
    }
}

void VulkanEngine::createCommandBuffer() {

    VkCommandBufferAllocateInfo allocInfo {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool = _commandPool,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = 1
    };

    if ( vkAllocateCommandBuffers( _device, &allocInfo, &_commandBuffer) != VK_SUCCESS ) {

        throw std::runtime_error("Failed to allocate command buffer");
    }
}

uint VulkanEngine::findMemoryType(uint typeFilter, VkMemoryPropertyFlags desiredProperties) {

    VkPhysicalDeviceMemoryProperties memProps;
    vkGetPhysicalDeviceMemoryProperties( _physicalDevice, &memProps );

    for ( uint i = 0; i < memProps.memoryTypeCount; i++ ) {

        if ( typeFilter & (1 << i) && (memProps.memoryTypes[i].propertyFlags & desiredProperties) ) {
            return i;
        }
    }

    throw std::runtime_error("failed to find suitable memory type");
}

void VulkanEngine::createVertexBuffer() {

    const std::vector<Vertex> vertices = {

        { {0.0f, -.5f}, {1.0f, 0.0f, 0.0f} },
        { {0.5f, 0.5f}, {0.0f, 1.0f, 0.0f} },
        { {-.5f, 0.5f}, {0.0f, 0.0f, 1.0f} }
    };

    // Initialize buffer
    uint bufferSize = sizeof(Vertex) * vertices.size();

    {
        VkBufferCreateInfo createInfo {
            .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
            .size = bufferSize,
            .usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
            .sharingMode = VK_SHARING_MODE_EXCLUSIVE
        };

        if ( vkCreateBuffer( _device, &createInfo, nullptr, &_vertexBuffer) != VK_SUCCESS ) {

            throw std::runtime_error("Failed to create vertex buffer");
        }
    }

    // Alloc memory and bind it to the buffer
    {
        VkMemoryRequirements memRequirements;
        vkGetBufferMemoryRequirements( _device, _vertexBuffer, &memRequirements );

        VkMemoryAllocateInfo allocInfo {
            .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
            .allocationSize = memRequirements.size,
            .memoryTypeIndex = findMemoryType( memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT )
        };

        if ( vkAllocateMemory( _device, &allocInfo, nullptr, &_vertexBufferMemory) != VK_SUCCESS ) {

            throw std::runtime_error("Unable to allocate vertex buffer memory");
        }

        vkBindBufferMemory( _device, _vertexBuffer, _vertexBufferMemory, 0 );
    }

    void *data;
    vkMapMemory( _device, _vertexBufferMemory, 0, bufferSize, 0, &data);

    memcpy( data, vertices.data(), bufferSize );

    vkUnmapMemory( _device, _vertexBufferMemory );
}
    
void VulkanEngine::recordCommandBuffer( VkCommandBuffer commandBuffer, uint imageIndex ) {

    VkCommandBufferBeginInfo beginInfo {

        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = 0,
        .pInheritanceInfo = nullptr
    };

    if ( vkBeginCommandBuffer( commandBuffer, &beginInfo ) != VK_SUCCESS ) {

        throw std::runtime_error("Failed to begin recording command buffer");
    }

    VkClearValue clearColors = { 
        { { .6f, .6f, .6f, 1.f } }
    };

    VkRenderPassBeginInfo renderPassInfo {
        
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
        .renderPass = _renderPass,
        .framebuffer = _swapchainFramebuffers[imageIndex],
        .renderArea = {
            .offset = {0, 0},
            .extent = _swapchainExtent,
        },
        .clearValueCount = 1,
        .pClearValues = &clearColors
    };

    vkCmdBeginRenderPass( commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE );

    vkCmdBindPipeline( commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, _mainGraphicsPipeline );

    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(std::round(_swapchainExtent.width));
    viewport.height = static_cast<float>(std::round(_swapchainExtent.height));
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    vkCmdSetViewport( commandBuffer, 0, 1, &viewport );

    VkRect2D scissor{};
    scissor.offset = {0, 0};
    scissor.extent = _swapchainExtent;
    vkCmdSetScissor( commandBuffer, 0, 1, &scissor );

    {
        VkBuffer vertexBuffers[] = { _vertexBuffer };
        VkDeviceSize offsets[] = { 0 };
        vkCmdBindVertexBuffers(_commandBuffer, 0, 1, vertexBuffers, offsets );

        vkCmdDraw( _commandBuffer, 3, 1, 0, 0 );
    }

    vkCmdEndRenderPass( commandBuffer );

    if ( vkEndCommandBuffer( commandBuffer ) != VK_SUCCESS ) {

        throw std::runtime_error("Failed to record command buffer");
    }
}

void VulkanEngine::createSyncObjects() {

    VkSemaphoreCreateInfo semaphoreInfo{
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
    };

    VkFenceCreateInfo fenceInfo{
        .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
        .flags = VK_FENCE_CREATE_SIGNALED_BIT
    };

    if ( vkCreateSemaphore( _device, &semaphoreInfo, nullptr, &_imageAvailableSemaphore ) != VK_SUCCESS ||
        vkCreateSemaphore( _device, &semaphoreInfo, nullptr, &_renderFinishedSemaphore ) != VK_SUCCESS ) {

        throw std::runtime_error("Unable to create semaphore");
    }

    if ( vkCreateFence( _device, &fenceInfo, nullptr, &_inFlightFence) != VK_SUCCESS ) {

        throw std::runtime_error("Unable to create fence");
    } 
}

void VulkanEngine::drawFrame() {

    // Wait for previous frame to be rendered
    vkWaitForFences( _device, 1, &_inFlightFence, VK_TRUE, UINT64_MAX );
    vkResetFences( _device, 1, &_inFlightFence );

    uint imageIndex;

    vkAcquireNextImageKHR( _device, _swapchain, UINT64_MAX, _imageAvailableSemaphore, VK_NULL_HANDLE, &imageIndex );

    // Record new commands
    vkResetCommandBuffer( _commandBuffer, 0 );
    recordCommandBuffer( _commandBuffer, imageIndex );

    // Submit command buffer to queue
    VkSemaphore waitSemaphore[] = { _imageAvailableSemaphore };
    VkSemaphore signalSemaphores[] { _renderFinishedSemaphore };
    VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };

    VkSubmitInfo submitInfo { 
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = waitSemaphore,
        .pWaitDstStageMask = waitStages,
        .commandBufferCount = 1,
        .pCommandBuffers = &_commandBuffer,
        .signalSemaphoreCount = 1,
        .pSignalSemaphores = signalSemaphores,
    };

    if ( vkQueueSubmit( _graphicsQueue, 1, &submitInfo, _inFlightFence) != VK_SUCCESS ) {

        throw std::runtime_error("Failed to submit draw command buffer");
    }

    VkSwapchainKHR swapChains[] = { _swapchain };
    VkPresentInfoKHR presentInfo { 
        .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = signalSemaphores,
        .swapchainCount = 1,
        .pSwapchains = swapChains,
        .pImageIndices = &imageIndex,
        .pResults = nullptr,
    };

    vkQueuePresentKHR( _presentQueue, &presentInfo );
}

void VulkanEngine::deviceWaitIdle() {

    vkDeviceWaitIdle( _device );
}

void VulkanEngine::release() {

    vkDestroyBuffer( _device, _vertexBuffer, nullptr );
    _vertexBuffer = nullptr;

    vkFreeMemory( _device, _vertexBufferMemory, nullptr );
    _vertexBufferMemory = nullptr;

    vkDestroySemaphore( _device, _imageAvailableSemaphore, nullptr);
    _imageAvailableSemaphore = nullptr;

    vkDestroySemaphore( _device, _renderFinishedSemaphore, nullptr);
    _renderFinishedSemaphore = nullptr;

    vkDestroyFence( _device, _inFlightFence, nullptr );
    _inFlightFence = nullptr;

    vkDestroyCommandPool( _device, _commandPool, nullptr );
    _commandPool = nullptr;

    for(auto framebuffer : _swapchainFramebuffers) {

        vkDestroyFramebuffer( _device, framebuffer, nullptr );
    }

    _swapchainFramebuffers.clear();

    vkDestroyPipeline( _device, _mainGraphicsPipeline, nullptr );
    _mainGraphicsPipeline = nullptr;

    vkDestroyPipelineLayout( _device, _pipelineLayout, nullptr );
    _pipelineLayout = nullptr;

    vkDestroyRenderPass( _device, _renderPass, nullptr );
    _renderPass = nullptr;

    _mainShader.release();

    for(auto imageView : _swapchainImageViews) {

        vkDestroyImageView( _device, imageView, nullptr );
    }

    _swapchainImageViews.clear();

    vkDestroySwapchainKHR( _device, _swapchain, nullptr );
    _swapchain = nullptr;

    vkDestroySurfaceKHR( _instance, _surface, nullptr );
    _surface = nullptr;

    vkDestroyDevice( _device, nullptr );
    _device = nullptr;

    vkDestroyInstance( _instance, nullptr );
    _instance = nullptr;

    _safe = false;
}

bool VulkanEngine::isSafe()
{
    return _safe;
}
