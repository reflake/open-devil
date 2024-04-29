#include <vulkan/vulkan_core.h>
#include <vulkan/vulkan.h>

#include <vector>

// should be released after use
class Shader {

public:

    Shader( VkDevice device, VkShaderModule vertShaderModule, VkShaderModule fragShaderModule ) : 
        _device(device),
        _vertShaderModule(vertShaderModule), 
        _fragShaderModule(fragShaderModule) {}

    void release();

    VkShaderModule getVertexShaderModule();
    VkShaderModule getFragmentShaderModule();

    static Shader loadShader( VkDevice device, const char *vertPath, const char *fragPath );
    static VkShaderModule createShaderModule( VkDevice device, const std::vector<char>& code );

private:

    VkDevice _device;
    VkShaderModule _vertShaderModule;
    VkShaderModule _fragShaderModule;
    static std::vector<char> readFile(const char *filepath);
};