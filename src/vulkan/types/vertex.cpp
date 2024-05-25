#include "vertex.hpp"
#include <array>
#include <vulkan/vulkan_core.h>

VkVertexInputBindingDescription Vertex::getBindingDescription() {

	return {
		.binding = 0,
		.stride = sizeof(Vertex),
		.inputRate = VK_VERTEX_INPUT_RATE_VERTEX
	};
}

std::array<VkVertexInputAttributeDescription, 3> Vertex::getAttributeDescription() {

	return {
		VkVertexInputAttributeDescription{
			.location = 0,
			.binding = 0,
			.format = VK_FORMAT_R32G32_SFLOAT,
			.offset = offsetof( Vertex, pos )
		},
		{
			.location = 1,
			.binding = 0,
			.format = VK_FORMAT_R8G8B8_UNORM,
			.offset = offsetof( Vertex, color )
		},
		{
			.location = 2,
			.binding = 0,
			.format = VK_FORMAT_R32G32_SFLOAT,
			.offset = offsetof( Vertex, uv0 )
		}
	};
}