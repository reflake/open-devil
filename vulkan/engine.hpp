#pragma once

#include <SDL2/SDL_video.h>
#include <vector>
#include <vulkan/vulkan.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_vulkan.h>
#include <optional>
#include <vulkan/vulkan_core.h>

extern const bool enableValidationLayers;

struct QueueFamilyIndices {
    std::optional<uint> graphicsFamily;
    std::optional<uint> presentFamily;

    bool isComplete() {
        return graphicsFamily.has_value() && presentFamily.has_value();
    }
};

struct SwapChainSupportDetails {

    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;

    bool isComplete() {
        return !formats.empty() && !presentModes.empty();
    }
};

class VulkanEngine {

public:

    VulkanEngine() {}

    void setup(SDL_Window* window);
    bool isSafe();
    void release();

private:

    void createInstance( SDL_Window* window );
    bool checkValidationLayerSupport();
    void pickPhysicalDevice();
    bool isSuitableDevice( VkPhysicalDevice device );
    bool checkDeviceExtensionsSupported( VkPhysicalDevice device );
    SwapChainSupportDetails querySwapChainSupport( VkPhysicalDevice device );
    QueueFamilyIndices findQueueFamilies( VkPhysicalDevice device );
    VkSurfaceFormatKHR chooseSwapSurfaceFormat( const std::vector<VkSurfaceFormatKHR>& availableFormats);
    VkPresentModeKHR chooseSwapPresentMode( const std::vector<VkPresentModeKHR>& availablePresentModes);
    VkExtent2D chooseSwapExtent( const VkSurfaceCapabilitiesKHR& capabilities);
    void createLogicalDevice();
    void createSwapChain();
    void getDeviceQueue();
    void createWindowSurface( SDL_Window* window );

private:

    VkInstance _instance = VK_NULL_HANDLE;
    VkPhysicalDevice _physicalDevice = VK_NULL_HANDLE;
    VkDevice _device;
    VkQueue _graphicsQueue;
    VkQueue _presentQueue;
    VkSurfaceKHR _surface;
    VkSwapchainKHR _swapchain;
    SDL_Window* _window;
    bool _safe = false;
};