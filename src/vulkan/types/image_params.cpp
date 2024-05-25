#include <vulkan/vulkan_core.h>
#include "image_params.hpp"

ImageParams samplerImageParams {
    .optFormat = VK_FORMAT_R8G8B8A8_SRGB,
    .optTiling = VK_IMAGE_TILING_OPTIMAL,
    .optUsageFlags = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
 };

#define overrideField(a) .a = newParams.a.has_value() ? newParams.a.value() : this->a

ImageParams ImageParams::Overriden(ImageParams newParams) {

    return {
        overrideField(optFormat),
        overrideField(optTiling),
        overrideField(optUsageFlags)
    };
};

void ImageParams::validate() {
    // TODO: validation
}