#include "qfamily_indices.hpp"

bool QueueFamilyIndices::isComplete() {
	return graphicsFamily.has_value() && presentFamily.has_value();
}

QueueFamilyIndices findQueueFamilies( VkPhysicalDevice device, VkSurfaceKHR surface ) {

	uint queueFamilyCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties( device, &queueFamilyCount, nullptr );

	vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
	vkGetPhysicalDeviceQueueFamilyProperties( device, &queueFamilyCount, queueFamilies.data() );

	QueueFamilyIndices indices;
	int index = 0;

	for ( const auto& queueFamily : queueFamilies ) {

		if ( queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT ) {

			indices.graphicsFamily = index;
		}

		VkBool32 presentSupport = false;

		vkGetPhysicalDeviceSurfaceSupportKHR( device, index, surface, &presentSupport);

		if ( presentSupport ) {

			indices.presentFamily = index;
		}

		index++;
	}

	return indices;
}