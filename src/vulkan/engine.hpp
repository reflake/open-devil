#pragma once

#include <SDL2/SDL.h>
#include <SDL2/SDL_video.h>
#include <SDL2/SDL_vulkan.h>

#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>

#include <vector>

#include "types/qfamily_indices.hpp"
#include "types/swap_chain_support.hpp"
#include "shader.hpp"

using std::vector;

extern const bool enableValidationLayers;

class VulkanEngine {

public:

    VulkanEngine() {
        _swapchainImages = vector<VkImage>();
        _swapchainImageViews = vector<VkImageView>();
        _swapchainFramebuffers = vector<VkFramebuffer>();
    }

    void setup(SDL_Window* window);
    void drawFrame();
    void deviceWaitIdle();
    bool isSafe();
    void release();

private:

    void createInstance( SDL_Window* window );
    bool checkValidationLayerSupport();
    void createWindowSurface( SDL_Window* window );
    void pickPhysicalDevice();
    bool isSuitableDevice( VkPhysicalDevice device );
    bool checkDeviceExtensionsSupported( VkPhysicalDevice device );
    VkSurfaceFormatKHR chooseSwapSurfaceFormat( const std::vector<VkSurfaceFormatKHR>& availableFormats);
    VkPresentModeKHR chooseSwapPresentMode( const std::vector<VkPresentModeKHR>& availablePresentModes);
    VkExtent2D chooseSwapExtent( const VkSurfaceCapabilitiesKHR& capabilities);
    void createLogicalDevice();
    void createSwapChain();
    void createImageViews();
    void createRenderPass();
    void createRenderPipeline();
    void createFramebuffers();
    void createCommandPool();
    void createCommandBuffer();
    void createVertexBuffer();
    void recordCommandBuffer( VkCommandBuffer commandBuffer, uint imageIndex );
    void createSyncObjects();
    void createBuffer( VkDeviceSize size, VkBufferUsageFlags usageFlags, VkMemoryPropertyFlags memProps, VkBuffer &buffer, VkDeviceMemory &bufferMemory );
    void copyBuffer( VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size );
    void createDescriptorSetlayout();
    void createUniformBuffer();
    void updateUniformBuffer();
    void createDescriptorPool();
    void allocDescriptorSet();
    uint findMemoryType(uint typeFilter, VkMemoryPropertyFlags props);

private:

    Shader _mainShader;
    VkInstance _instance = VK_NULL_HANDLE;
    VkPhysicalDevice _physicalDevice = VK_NULL_HANDLE;
    VkDevice _device;
    VkQueue _graphicsQueue;
    VkQueue _presentQueue;
    VkSurfaceKHR _surface;
    VkSwapchainKHR _swapchain;
    SDL_Window* _window;
    vector<VkImage> _swapchainImages;
    vector<VkImageView> _swapchainImageViews;
    VkFormat _swapchainImageFormat;
    VkExtent2D _swapchainExtent;
    VkPipelineLayout _pipelineLayout;
    VkRenderPass _renderPass;
    VkPipeline _mainGraphicsPipeline;
    vector<VkFramebuffer> _swapchainFramebuffers;
    VkCommandPool _commandPool;
    VkCommandBuffer _commandBuffer;
    VkSemaphore _imageAvailableSemaphore;
    VkSemaphore _renderFinishedSemaphore;
    VkFence _inFlightFence;
    VkBuffer _vertexBuffer;
    VkDeviceMemory _vertexBufferMemory;
    VkDescriptorSetLayout _descriptorSetLayout;
    VkBuffer _uniformBuffer;
    VkDeviceMemory _uniformBufferMemory;
    VkDescriptorPool _descriptorPool;
    VkDescriptorSet _descriptorSet;
    void *_uniformBufferMapped;
    bool _safe = false;
};