#pragma once

#include <vulkan/vulkan_core.h>

#include <optional>
#include <vector>
#include <sys/types.h>

using std::vector;
using std::optional;

struct QueueFamilyIndices {
	optional<uint> graphicsFamily;
	optional<uint> presentFamily;

	bool isComplete();
};

QueueFamilyIndices findQueueFamilies( VkPhysicalDevice device, VkSurfaceKHR surface );