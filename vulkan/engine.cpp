#include <SDL2/SDL_error.h>
#include <SDL2/SDL_vulkan.h>
#include <algorithm>
#include <stdexcept>
#include <vector>
#include <iostream>
#include <vulkan/vulkan_core.h>
#include <cstring>
#include <set>

#include "engine.hpp"

#ifdef NDEBUG
    const bool enableValidationLayers = false;
#else
    const bool enableValidationLayers = true;
#endif

const std::vector<const char*> validationLayers = {
    "VK_LAYER_KHRONOS_validation"  
};

bool isLayerEqual( const char *a, const char *b ) {

    return strcmp( a, b ) == 0;
}

void VulkanEngine::setup( SDL_Window* window ) {

    _safe = true;

    if (enableValidationLayers && !checkValidationLayerSupport()) {

        throw std::runtime_error("Vulkan API required validation layers unavailable");
    }

    createInstance( window );
    createWindowSurface( window );
    pickPhysicalDevice();
    createLogicalDevice();
    getDeviceQueue();
}

bool VulkanEngine::checkValidationLayerSupport() {

    uint propertyCount;

    vkEnumerateInstanceLayerProperties( &propertyCount, nullptr );

    std::vector<VkLayerProperties> availableLayerProps(propertyCount);
    vkEnumerateInstanceLayerProperties( &propertyCount, availableLayerProps.data());

    for (const char* validLayer : validationLayers) {

        bool layerFound = std::any_of( 
            availableLayerProps.begin(), 
            availableLayerProps.end(), 
            [validLayer] (const auto& layerProps) { return isLayerEqual( validLayer, layerProps.layerName ); } );

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

    std::vector<const char*> extensionNames(pCount);

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

void VulkanEngine::pickPhysicalDevice() {

    uint deviceCount;

    // Find the number of physical devices
    vkEnumeratePhysicalDevices( _instance, &deviceCount, nullptr );

    if ( deviceCount == 0 ) {

        throw std::runtime_error( "Failed to find GPUs with Vulkan support" );
        _safe = false;
    }

    std::vector<VkPhysicalDevice> devices(deviceCount);

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

    QueueFamilyIndices queueFamilyIndices = findQueueFamilies( device );

    
    return props.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU &&
           features.geometryShader &&
           queueFamilyIndices.isComplete();
}

QueueFamilyIndices VulkanEngine::findQueueFamilies(VkPhysicalDevice device) {

    uint queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties( device, &queueFamilyCount, nullptr );

    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties( device, &queueFamilyCount, queueFamilies.data() );

    QueueFamilyIndices indices;
    int index = 0;

    for ( const auto& queueFamily : queueFamilies ) {

        if ( queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT ) {

            indices.graphicsFamily = index;
        }

        VkBool32 presentSupport = false;

        vkGetPhysicalDeviceSurfaceSupportKHR( device, index, _surface, &presentSupport);

        if ( presentSupport ) {

            indices.presentFamily = index;
        }

        index++;
    }

    return indices;
}

void VulkanEngine::createLogicalDevice() {

    QueueFamilyIndices familyIndices = findQueueFamilies( _physicalDevice );

    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
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
        .enabledExtensionCount = 0,
        .pEnabledFeatures = &deviceFeatures,
    };

    if ( enableValidationLayers ) {

        deviceCreateInfo.enabledLayerCount = validationLayers.size();
        deviceCreateInfo.ppEnabledLayerNames = validationLayers.data();
    }

    if ( vkCreateDevice( _physicalDevice, &deviceCreateInfo, nullptr, &_device) != VK_SUCCESS ) {

        throw std::runtime_error("Vulkan API: Failed to create logical device");
    }
}

void VulkanEngine::getDeviceQueue() {

    QueueFamilyIndices indices = findQueueFamilies( _physicalDevice );

    vkGetDeviceQueue( _device, indices.graphicsFamily.value(), 0, &_graphicsQueue);
    vkGetDeviceQueue( _device, indices.presentFamily.value(), 0, &_presentQueue);
}

void VulkanEngine::createWindowSurface( SDL_Window* window ) {

    if ( SDL_Vulkan_CreateSurface( window, _instance, &_surface) == SDL_FALSE ) {

        throw std::runtime_error( "Failed to create Vulkan API window surface" );
    }
}

void VulkanEngine::release()
{
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
