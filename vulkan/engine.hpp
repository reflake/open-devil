#pragma once

#include <vulkan/vulkan.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_vulkan.h>
#include <optional>

extern const bool enableValidationLayers;

struct QueueFamilyIndices {
    std::optional<uint> graphicsFamily;
    std::optional<uint> presentFamily;

    bool isComplete() {
        return graphicsFamily.has_value() && presentFamily.has_value();
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
    QueueFamilyIndices findQueueFamilies( VkPhysicalDevice device );
    void createLogicalDevice();
    void getDeviceQueue();
    void createWindowSurface( SDL_Window* window );

private:

    VkInstance _instance = VK_NULL_HANDLE;
    VkPhysicalDevice _physicalDevice = VK_NULL_HANDLE;
    VkDevice _device;
    VkQueue _graphicsQueue;
    VkQueue _presentQueue;
    VkSurfaceKHR _surface;
    bool _safe = false;
};