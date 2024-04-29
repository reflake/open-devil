#include "shader.hpp"
#include "../file.hpp"
#include <cstdint>
#include <stdexcept>
#include <vulkan/vulkan_core.h>

Shader Shader::loadShader( VkDevice device, const char *vertPath, const char *fragPath ) {

    auto vertShaderCode = readFile( vertPath );
    auto fragShaderCode = readFile( fragPath );

    auto vertShaderModule = createShaderModule( device, vertShaderCode );
    auto fragShaderModule = createShaderModule( device, fragShaderCode );

    return Shader( device, vertShaderModule, fragShaderModule );
};

VkShaderModule Shader::createShaderModule( VkDevice device, const std::vector<char>& code ) {

    VkShaderModuleCreateInfo createInfo {
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .codeSize = code.size(),
        .pCode = reinterpret_cast<const uint32_t*>(code.data())
    };

    VkShaderModule shaderModule;

    if ( vkCreateShaderModule( device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS ) {

        throw std::runtime_error("Failed to create shader module");
    }

    return shaderModule;
};

std::vector<char> Shader::readFile(const char *filepath) {

    File file = File::openBinary( string(filepath) );
    int size = file.getSize();

    std::vector<char> chars(size);

    file.readBinary( size, chars.data() );
    file.close();

    return chars;
};    

VkShaderModule Shader::getVertexShaderModule() { return _vertShaderModule; }
    
VkShaderModule Shader::getFragmentShaderModule() { return _fragShaderModule; }

void Shader::release() {

    vkDestroyShaderModule( _device, _vertShaderModule, nullptr );
    vkDestroyShaderModule( _device, _fragShaderModule, nullptr );
};