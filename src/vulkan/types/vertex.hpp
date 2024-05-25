#pragma once

#include <glm/glm.hpp>
#include <vulkan/vulkan_core.h>

#include <array>

struct Vertex {
	glm::vec3 pos;
	glm::u8vec3 color;
	glm::vec2 uv0;

	static VkVertexInputBindingDescription getBindingDescription();
	static std::array<VkVertexInputAttributeDescription, 3> getAttributeDescription();
};