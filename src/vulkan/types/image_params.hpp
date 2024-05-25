#pragma once

#include <vulkan/vulkan_core.h>

#include <optional>

using std::optional;

struct ImageParams {
    optional<VkFormat> optFormat;
    optional<VkImageTiling> optTiling;
    optional<VkImageUsageFlags> optUsageFlags;

    ImageParams Overriden(ImageParams newParams);
    void validate();
};

extern ImageParams samplerImageParams;