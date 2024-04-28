#pragma once

#include <vulkan/vulkan_core.h>
#include <vector>
#include <sys/types.h>

using std::vector;

struct SwapChainSupportDetails {

    VkSurfaceCapabilitiesKHR capabilities;
    vector<VkSurfaceFormatKHR> formats;
    vector<VkPresentModeKHR> presentModes;

    bool isComplete();
};

SwapChainSupportDetails querySwapChainSupport( VkPhysicalDevice device, VkSurfaceKHR surface );