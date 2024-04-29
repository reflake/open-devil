#include <SDL2/SDL_video.h>
#include <SDL2/SDL_vulkan.h>

#include <vulkan/vulkan_core.h>

#include <algorithm>
#include <cstring>
#include <iostream>
#include <set>
#include <stdexcept>
#include <limits>

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

void VulkanEngine::release()
{
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