#pragma once

#include <glm/glm.hpp>
#include <vulkan/vulkan_core.h>

#include <array>

struct Vertex {
	glm::vec2 pos;
	glm::u8vec3 color;

	static VkVertexInputBindingDescription getBindingDescription();
	static std::array<VkVertexInputAttributeDescription, 2> getAttributeDescription();
};