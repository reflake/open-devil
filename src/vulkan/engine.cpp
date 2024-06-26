#include "engine.hpp"

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <vulkan/vulkan_core.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>
#include <iostream>
#include <set>
#include <stdexcept>
#include <limits>

#define GLM_FORCE_DEPTH_ZERO_TO_ONE

#include <glm/ext/vector_float3.hpp>
#include <glm/geometric.hpp>
#include <glm/trigonometric.hpp>
#include <glm/gtx/rotate_vector.hpp>

#include "types/uniform_buffer.hpp"
#include "types/qfamily_indices.hpp"
#include "types/vertex.hpp"
#include "../media/image.hpp"
#include "../media/model.hpp"

#ifdef NDEBUG
	const bool enableValidationLayers = false;
#else
	const bool enableValidationLayers = true;
#endif

const vector<const char*> validationLayers = {
	"VK_LAYER_KHRONOS_validation"  
};

const int MAX_FRAMES_IN_FLIGHT = 2;

bool isStrEqual( const char *a, const char *b ) {

	return strcmp( a, b ) == 0;
}

const vector<const char*> deviceExtensions = {
	VK_KHR_SWAPCHAIN_EXTENSION_NAME,
	VK_KHR_MAINTENANCE1_EXTENSION_NAME
};

void VulkanEngine::setup( SDL_Window* window ) {

	_safe = true;

	if (enableValidationLayers && !checkValidationLayerSupport()) {

		throw std::runtime_error("Vulkan API required validation layers unavailable");
	}

	_window = window;

	createInstance( window );
	createWindowSurface( window );
	pickPhysicalDevice();
	createLogicalDevice();
	createSwapChain();
	createSwapChainImageViews();
	createRenderPass();
	createDescriptorSetlayout();
	createRenderPipeline();
	createSyncObjects();
	createCommandPool();
	createCommandBuffers();
	createDepthResources();
	createFramebuffers();
	loadModel();
	createUniformBuffer();
	createDescriptorPool();
	createTextureSampler();
	allocDescriptorSets();
}

bool VulkanEngine::checkValidationLayerSupport() {

	uint propertyCount;

	vkEnumerateInstanceLayerProperties( &propertyCount, nullptr );

	vector<VkLayerProperties> availableLayerProps(propertyCount);
	vkEnumerateInstanceLayerProperties( &propertyCount, availableLayerProps.data());

	for (const char* validLayer : validationLayers) {

		bool layerFound = std::any_of( 
			availableLayerProps.begin(), 
			availableLayerProps.end(), 
			[validLayer] (const auto& layerProps) { return isStrEqual( validLayer, layerProps.layerName ); } );

		if ( !layerFound )
			return false;
	}

	return true;
}

void VulkanEngine::createInstance( SDL_Window* window ) {

	VkApplicationInfo appInfo{
		.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
		.pNext = NULL,
		.pApplicationName = "open-devil",
		.applicationVersion = 0,
		.pEngineName = "open-devil-engine",
		.engineVersion = 0,
		.apiVersion = VK_API_VERSION_1_0
	};

	// List required SDL extensions for Vulkan
	uint pCount;

	SDL_Vulkan_GetInstanceExtensions( window, &pCount, nullptr );

	vector<const char*> extensionNames(pCount);

	SDL_Vulkan_GetInstanceExtensions( window, &pCount, extensionNames.data() );

	VkInstanceCreateInfo instanceInfo{
		.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
		.pApplicationInfo = &appInfo,
		.enabledLayerCount = 0,
		.ppEnabledLayerNames = NULL,
		.enabledExtensionCount = pCount,
		.ppEnabledExtensionNames = extensionNames.data()
	};

	if ( enableValidationLayers ) {

		instanceInfo.enabledLayerCount = validationLayers.size();
		instanceInfo.ppEnabledLayerNames = validationLayers.data();
	}

	if ( vkCreateInstance(&instanceInfo, NULL, &_instance) != VK_SUCCESS ) {

		throw std::runtime_error("Failed to create Vulkan instance");
		_safe = false;
	}
}

void VulkanEngine::createWindowSurface( SDL_Window* window ) {

	if ( SDL_Vulkan_CreateSurface( window, _instance, &_surface) == SDL_FALSE ) {

		throw std::runtime_error( "Failed to create Vulkan API window surface" );
	}
}

void VulkanEngine::pickPhysicalDevice() {

	uint deviceCount;

	// Find the number of physical devices
	vkEnumeratePhysicalDevices( _instance, &deviceCount, nullptr );

	if ( deviceCount == 0 ) {

		throw std::runtime_error( "Failed to find GPUs with Vulkan support" );
		_safe = false;
	}

	vector<VkPhysicalDevice> devices(deviceCount);

	vkEnumeratePhysicalDevices( _instance, &deviceCount, devices.data() );

	// Search for a suitable device
	auto it = std::find_if( devices.begin(), 
							 devices.end(), 
							 [this](VkPhysicalDevice dev) { return isSuitableDevice(dev); } );

	if ( it == devices.end() ) {

		throw std::runtime_error( "Failed to find a suitable GPU" );
		_safe = false;
	}

	_physicalDevice = *it;

	// Print device name
	VkPhysicalDeviceProperties props;

	vkGetPhysicalDeviceProperties( _physicalDevice, &props );

	std::cout << "Device picked: (" << props.deviceID << ") " << props.deviceName << std::endl;
}

bool VulkanEngine::isSuitableDevice( VkPhysicalDevice device ) {

	VkPhysicalDeviceProperties props;
	VkPhysicalDeviceFeatures features;

	vkGetPhysicalDeviceProperties( device, &props );
	vkGetPhysicalDeviceFeatures( device, &features );

	QueueFamilyIndices queueFamilyIndices = findQueueFamilies( device, _surface );

	bool areExtensionsSupported = checkDeviceExtensionsSupported( device );
	bool swapChainAdequate = false;

	if ( areExtensionsSupported ) {

		auto swapChainSupport = querySwapChainSupport( device, _surface );
		swapChainAdequate = swapChainSupport.isComplete();
	}

	return props.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU &&
		   features.geometryShader &&
		   features.samplerAnisotropy &&
		   areExtensionsSupported && 
		   swapChainAdequate &&
		   queueFamilyIndices.isComplete();
}

bool VulkanEngine::checkDeviceExtensionsSupported( VkPhysicalDevice device ) {

	uint extensionsCount;
	vkEnumerateDeviceExtensionProperties( device, nullptr, &extensionsCount, nullptr);

	vector<VkExtensionProperties> availableExtensions( extensionsCount );
	vkEnumerateDeviceExtensionProperties( device, nullptr, &extensionsCount, availableExtensions.data() );

	for(const char *requiredExtension : deviceExtensions) {

		bool extensionFound = std::any_of(
			availableExtensions.begin(),
			availableExtensions.end(),
			[requiredExtension] ( const auto availableExtension ) { return isStrEqual( requiredExtension, availableExtension.extensionName ); }
		);

		if (!extensionFound)
			return false;
	}

	return true;
}

VkSurfaceFormatKHR VulkanEngine::chooseSwapSurfaceFormat( const vector<VkSurfaceFormatKHR>& availableFormats ) {

	for ( const auto& availableFormat : availableFormats) {

		if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {

			return availableFormat;
		}
	}

	return availableFormats[0];
}

VkPresentModeKHR VulkanEngine::chooseSwapPresentMode( const vector<VkPresentModeKHR>& availableModes ) {

	auto it = std::find( availableModes.begin(), availableModes.end(), VK_PRESENT_MODE_MAILBOX_KHR );

	if ( it != availableModes.end() ) {

		return *it;
	}

	// Default fallback mode
	return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D VulkanEngine::chooseSwapExtent( const VkSurfaceCapabilitiesKHR& capabilities) {

	if ( capabilities.currentExtent.width != std::numeric_limits<uint>::max() ) {

		return capabilities.currentExtent;
	}

	int width, height;
	SDL_GL_GetDrawableSize( _window, &width, &height );

	VkExtent2D actualExtent = {
		static_cast<uint>(width),
		static_cast<uint>(height)
	};

	actualExtent.width = std::clamp( actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width );
	actualExtent.height = std::clamp( actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height );

	return actualExtent;
}

void VulkanEngine::createLogicalDevice() {

	QueueFamilyIndices familyIndices = findQueueFamilies( _physicalDevice, _surface );

	vector<VkDeviceQueueCreateInfo> queueCreateInfos;
	std::set<uint> uniqueQueueFamilies = { 
		familyIndices.graphicsFamily.value(), 
		familyIndices.presentFamily.value() };

	float queuePriorities[1] = { 1.0f };

	for (uint queueFamilyIndex : uniqueQueueFamilies ) {

		VkDeviceQueueCreateInfo createInfo {
			.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
			.queueFamilyIndex = queueFamilyIndex,
			.queueCount = 1,
			.pQueuePriorities = queuePriorities
		};

		queueCreateInfos.push_back( createInfo );
	}

	VkPhysicalDeviceFeatures deviceFeatures{};
	deviceFeatures.samplerAnisotropy = VK_TRUE;

	VkDeviceCreateInfo deviceCreateInfo {
		.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
		.queueCreateInfoCount = static_cast<uint>(queueCreateInfos.size()),
		.pQueueCreateInfos = queueCreateInfos.data(),
		.enabledLayerCount = 0,
		.enabledExtensionCount = static_cast<uint>(deviceExtensions.size()),
		.ppEnabledExtensionNames = deviceExtensions.data(),
		.pEnabledFeatures = &deviceFeatures,
	};

	if ( enableValidationLayers ) {

		deviceCreateInfo.enabledLayerCount = validationLayers.size();
		deviceCreateInfo.ppEnabledLayerNames = validationLayers.data();
	}

	if ( vkCreateDevice( _physicalDevice, &deviceCreateInfo, nullptr, &_device) != VK_SUCCESS ) {

		throw std::runtime_error("Vulkan API: Failed to create logical device");
	}

	vkGetDeviceQueue( _device, familyIndices.graphicsFamily.value(), 0, &_graphicsQueue);
	vkGetDeviceQueue( _device, familyIndices.presentFamily.value(), 0, &_presentQueue);
}

void VulkanEngine::createSwapChain() {

	SwapChainSupportDetails supportDetails = querySwapChainSupport( _physicalDevice, _surface );

	auto surfaceFormat = chooseSwapSurfaceFormat( supportDetails.formats );
	auto presentMode = chooseSwapPresentMode( supportDetails.presentModes );
	auto extent = chooseSwapExtent( supportDetails.capabilities );

	// Find image count between min and max image count possible
	uint imageCount = supportDetails.capabilities.minImageCount + 1;

	if ( supportDetails.capabilities.maxImageCount > 0 && imageCount > supportDetails.capabilities.maxImageCount ) {

		imageCount = supportDetails.capabilities.maxImageCount;
	}

	VkSwapchainCreateInfoKHR createInfo{
		.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
		.surface = _surface,
		.minImageCount = imageCount,
		.imageFormat = surfaceFormat.format,
		.imageColorSpace = surfaceFormat.colorSpace,
		.imageExtent = extent,
		.imageArrayLayers = 1,
		.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
		.preTransform = supportDetails.capabilities.currentTransform,
		.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
		.presentMode = presentMode,
		.clipped = VK_TRUE,
		.oldSwapchain = VK_NULL_HANDLE,
	};

	QueueFamilyIndices queueFamilyIndices = findQueueFamilies( _physicalDevice, _surface );
	uint queueFamilyIndicesArray[] = { queueFamilyIndices.graphicsFamily.value(), queueFamilyIndices.presentFamily.value() };

	if ( queueFamilyIndices.graphicsFamily == queueFamilyIndices.presentFamily ) {

		createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		createInfo.queueFamilyIndexCount = 0;
		createInfo.pQueueFamilyIndices = nullptr;
	} else {
	
		createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		createInfo.queueFamilyIndexCount = 2;
		createInfo.pQueueFamilyIndices = queueFamilyIndicesArray;
	}

	if ( vkCreateSwapchainKHR( _device, &createInfo, nullptr, &_swapchain ) != VK_SUCCESS ) {

		throw std::runtime_error( "Vulkan API: failed to create swap chain" );
	}

	vkGetSwapchainImagesKHR( _device, _swapchain, &imageCount, nullptr );
	_swapchainImages.resize( imageCount );

	vkGetSwapchainImagesKHR( _device, _swapchain, &imageCount, _swapchainImages.data() );

	_swapchainImageFormat = surfaceFormat.format;
	_swapchainExtent = extent;
}

void VulkanEngine::createSwapChainImageViews() {

	for (auto swapchainImage : _swapchainImages) {

		VkImageView imageView;

		if ( createImageView( swapchainImage, _swapchainImageFormat, VK_IMAGE_ASPECT_COLOR_BIT, &imageView ) != VK_SUCCESS ) {

			throw std::runtime_error( "Could not create image view" );
		}

		_swapchainImageViews.push_back( imageView );
	}
}

void VulkanEngine::createRenderPass() {

	VkAttachmentDescription colorAttachment {
		.format = _swapchainImageFormat,
		.samples = VK_SAMPLE_COUNT_1_BIT,
		.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
		.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
		.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
		.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
		.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
		.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
	};

	VkAttachmentReference colorAttachmentRef {
		.attachment = 0,
		.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
	};

	VkAttachmentDescription depthAttachment {
		.format = findDepthFormat(),
		.samples = VK_SAMPLE_COUNT_1_BIT,
		.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
		.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
		.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
		.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
		.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
		.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
	};

	VkAttachmentReference depthAttachmentRef {
		.attachment = 1,
		.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
	};

	VkSubpassDescription subpass {
		.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
		.colorAttachmentCount = 1,
		.pColorAttachments = &colorAttachmentRef,
		.pDepthStencilAttachment = &depthAttachmentRef
	};

	VkSubpassDependency dependency { 
		.srcSubpass = VK_SUBPASS_EXTERNAL, 
		.dstSubpass = 0,
		.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
		.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
		.srcAccessMask = 0,
		.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
	};

	std::array<VkAttachmentDescription, 2> attachments = { colorAttachment, depthAttachment };

	VkRenderPassCreateInfo createInfo {
		.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
		.attachmentCount = attachments.size(),
		.pAttachments = attachments.data(),
		.subpassCount = 1,
		.pSubpasses = &subpass,
		.dependencyCount = 1,
		.pDependencies = &dependency
	};

	if ( vkCreateRenderPass( _device, &createInfo, nullptr, &_renderPass ) != VK_SUCCESS ) {

		throw std::runtime_error( "Failed to create render pass!" );
	}
}

void VulkanEngine::createRenderPipeline() {

	_mainShader = Shader::loadShader( _device, "shaders/main.vert.spv", "shaders/main.frag.spv");

	VkPipelineShaderStageCreateInfo vertShaderStageInfo {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
		.stage = VK_SHADER_STAGE_VERTEX_BIT,
		.module = _mainShader.getVertexShaderModule(),
		.pName = "main"
	};

	VkPipelineShaderStageCreateInfo fragShaderStageInfo {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
		.stage = VK_SHADER_STAGE_FRAGMENT_BIT,
		.module = _mainShader.getFragmentShaderModule(),
		.pName = "main"
	};

	VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };

	vector<VkDynamicState> dynamicStates = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };

	VkPipelineDynamicStateCreateInfo dynamicState {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
		.dynamicStateCount = static_cast<uint>(dynamicStates.size()),
		.pDynamicStates = dynamicStates.data()
	};

	// Vertex input attribute description
	auto bindingDesc = Vertex::getBindingDescription();
	auto attrsDesc = Vertex::getAttributeDescription();

	VkPipelineVertexInputStateCreateInfo vertexInputInfo {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
		.vertexBindingDescriptionCount = 1,
		.pVertexBindingDescriptions = &bindingDesc,
		.vertexAttributeDescriptionCount = attrsDesc.size(),
		.pVertexAttributeDescriptions = attrsDesc.data(),
	};

	VkPipelineInputAssemblyStateCreateInfo inputAssembly {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
		.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
		.primitiveRestartEnable = VK_FALSE,  
	};

	float width = static_cast<float>(std::round(_swapchainExtent.width));
	float height = static_cast<float>(std::round(_swapchainExtent.height));

	VkViewport viewport {
		.x = 0.f,
		.y = height, // invert y
		.width = width,
		.height = -height, // invert y
		.minDepth = 0.f,
		.maxDepth = 1.f
	};

	VkRect2D scissor {
		.offset = {0, 0},
		.extent = _swapchainExtent
	};

	VkPipelineViewportStateCreateInfo viewportState {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
		.viewportCount = 1,
		.pViewports = &viewport,
		.scissorCount = 1,
		.pScissors = &scissor
	};

	VkPipelineRasterizationStateCreateInfo rasterizer{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
		.depthClampEnable = VK_FALSE,
		.rasterizerDiscardEnable = VK_FALSE,
		.polygonMode = VK_POLYGON_MODE_FILL,
		.cullMode = VK_CULL_MODE_BACK_BIT,
		.frontFace = VK_FRONT_FACE_CLOCKWISE,
		.depthBiasEnable = VK_FALSE,
		.depthBiasConstantFactor = 0.f,
		.depthBiasClamp = 0.f,
		.depthBiasSlopeFactor = 0.f,
		.lineWidth = 1.f,
	};

	VkPipelineMultisampleStateCreateInfo multisampling {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
		.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
		.sampleShadingEnable = VK_FALSE,
		.minSampleShading = 1.f,
		.pSampleMask = nullptr,
		.alphaToCoverageEnable = VK_FALSE,
		.alphaToOneEnable = VK_FALSE
	};

	VkPipelineColorBlendAttachmentState colorBlendAttachment {
		.blendEnable = VK_FALSE,
		.srcColorBlendFactor = VK_BLEND_FACTOR_ONE,
		.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO,
		.colorBlendOp = VK_BLEND_OP_ADD,
		.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
		.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
		.alphaBlendOp = VK_BLEND_OP_ADD,
		.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
	};

	VkPipelineColorBlendStateCreateInfo colorBlending {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
		.logicOpEnable = VK_FALSE,
		.attachmentCount = 1,
		.pAttachments = &colorBlendAttachment,
		.blendConstants = { 0.f, 0.f, 0.f, 0.f }
	};

	VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
		.setLayoutCount = 1,
		.pSetLayouts = &_descriptorSetLayout,
		.pushConstantRangeCount = 0,
		.pPushConstantRanges = nullptr,
	};

	if ( vkCreatePipelineLayout( _device, &pipelineLayoutCreateInfo, nullptr, &_pipelineLayout ) != VK_SUCCESS ) {

		throw std::runtime_error("Unable to create pipeline layout");
	}

	VkPipelineDepthStencilStateCreateInfo depthStencil {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
		.depthTestEnable = VK_TRUE,
		.depthWriteEnable = VK_TRUE,
		.depthCompareOp = VK_COMPARE_OP_LESS,
		.depthBoundsTestEnable = VK_FALSE,
		.stencilTestEnable = VK_FALSE,
		.front = {},
		.back = {},
		.minDepthBounds = 0.0f,
		.maxDepthBounds = 1.0f,
	};

	VkGraphicsPipelineCreateInfo pipelineCreateInfo {
		.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
		.stageCount = 2,
		.pStages = shaderStages,
		.pVertexInputState = &vertexInputInfo,
		.pInputAssemblyState = &inputAssembly,
		.pViewportState = &viewportState,
		.pRasterizationState = &rasterizer,
		.pMultisampleState = &multisampling,
		.pDepthStencilState = &depthStencil,
		.pColorBlendState = &colorBlending,
		.pDynamicState = &dynamicState,
		.layout = _pipelineLayout,
		.renderPass = _renderPass,
		.subpass = 0,
		.basePipelineHandle = VK_NULL_HANDLE,
		.basePipelineIndex = -1,
	};

	if ( vkCreateGraphicsPipelines( _device, VK_NULL_HANDLE, 1, &pipelineCreateInfo, nullptr, &_mainGraphicsPipeline ) != VK_SUCCESS ) {

		throw std::runtime_error("Failed to create graphics pipeline");
	}
}

void VulkanEngine::createFramebuffers() {

	for ( auto iImageView : _swapchainImageViews ) {

		std::array<VkImageView, 2> attachments = { iImageView, _depthImageView };

		VkFramebufferCreateInfo createInfo {
			.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
			.renderPass = _renderPass,
			.attachmentCount = attachments.size(),
			.pAttachments = attachments.data(),
			.width = _swapchainExtent.width,
			.height = _swapchainExtent.height,
			.layers = 1
		};

		VkFramebuffer framebuffer;

		if ( vkCreateFramebuffer( _device, &createInfo, nullptr, &framebuffer ) != VK_SUCCESS ) {

			throw std::runtime_error("Failed to create framebuffer");
		}

		_swapchainFramebuffers.push_back( framebuffer );
	}
}

void VulkanEngine::createCommandPool() {

	auto queueFamilyIndices = findQueueFamilies( _physicalDevice, _surface );

	VkCommandPoolCreateInfo createInfo {
		.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
		.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
		.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value()
	};

	if ( vkCreateCommandPool( _device, &createInfo, nullptr, &_commandPool) != VK_SUCCESS ) {

		throw std::runtime_error("Failed to create command pool");
	}
}

void VulkanEngine::createCommandBuffers() {

	// Allocate multiple buffers for each frame in flight
	_commandBuffers.resize( MAX_FRAMES_IN_FLIGHT );

	VkCommandBufferAllocateInfo allocInfo {
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
		.commandPool = _commandPool,
		.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
		.commandBufferCount = MAX_FRAMES_IN_FLIGHT
	};

	if ( vkAllocateCommandBuffers( _device, &allocInfo, _commandBuffers.data() ) != VK_SUCCESS ) {

		throw std::runtime_error("Failed to allocate command buffers");
	}
}

uint VulkanEngine::findMemoryType(uint typeFilter, VkMemoryPropertyFlags desiredProperties) {

	VkPhysicalDeviceMemoryProperties memProps;
	vkGetPhysicalDeviceMemoryProperties( _physicalDevice, &memProps );

	for ( uint i = 0; i < memProps.memoryTypeCount; i++ ) {

		if ( typeFilter & (1 << i) && (memProps.memoryTypes[i].propertyFlags & desiredProperties) ) {
			return i;
		}
	}

	throw std::runtime_error("failed to find suitable memory type");
}

void VulkanEngine::loadModel() {

	auto model = readModel("models/vergil.fbx");

	// Initialize vertex buffer
	{
		int vertexCount = model.vertPositions.size();
		uint bufferSize = sizeof( Vertex ) * vertexCount;
		std::vector<Vertex> vertices( vertexCount );

		// Populate vertices list
		glm::u8vec3 whiteColor = {255, 255, 255};

		for( int i = 0; i < vertexCount; i++ ) {

			vertices[i] = { model.vertPositions[i], whiteColor, model.texCoords[i] };
		}

		VkBuffer stagingBuffer;
		VkDeviceMemory stagingBufferMemory;
		
		createBuffer(
			bufferSize, 
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT, 
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, 
			stagingBuffer, stagingBufferMemory);

		void *data;
		vkMapMemory( _device, stagingBufferMemory, 0, bufferSize, 0, &data);

		memcpy( data, vertices.data(), bufferSize );

		vkUnmapMemory( _device, stagingBufferMemory );

		createBuffer( 
			bufferSize, 
			VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, 
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 
			_vertexBuffer, _vertexBufferMemory);
		copyBuffer( stagingBuffer, _vertexBuffer, bufferSize);

		vkDestroyBuffer( _device, stagingBuffer, nullptr );
		vkFreeMemory( _device, stagingBufferMemory, nullptr );
	}

	// Initialize index buffer 
	{
		int indicesCount = model.indices.size();
		vector<uint16_t> indices( indicesCount );

		_numberOfIndices = indicesCount;

		for( int i = 0; i < indicesCount; i++ ) {

			indices[i] = model.indices[i];
		}

		uint bufferSize = sizeof(uint16_t) * indices.size();

		VkBuffer stagingBuffer;
		VkDeviceMemory stagingBufferMemory;

		createBuffer( bufferSize, 
				VK_BUFFER_USAGE_TRANSFER_SRC_BIT, 
				VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, 
				stagingBuffer, stagingBufferMemory);

		void* data;
		vkMapMemory( _device, stagingBufferMemory, 0, bufferSize, 0, &data );
		memcpy( data, indices.data(), bufferSize );
		vkUnmapMemory( _device, stagingBufferMemory );

		createBuffer( bufferSize, 
				VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, 
				VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 
				_indexBuffer, _indexBufferMemory );

		copyBuffer( stagingBuffer, _indexBuffer, bufferSize );

		vkDestroyBuffer( _device, stagingBuffer, nullptr );
		vkFreeMemory( _device, stagingBufferMemory, nullptr );
	}

	createTextureImage( Image::loadFile( model.texturePath.c_str() ));
	createTextureImageView();

}

void VulkanEngine::copyBuffer( VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size ) {

	// TODO: replace with singleTimeCmmands()
	VkCommandBufferAllocateInfo allocInfo{
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
		.commandPool = _commandPool,
		.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
		.commandBufferCount = 1,
	};

	VkCommandBuffer commandBuffer;
	vkAllocateCommandBuffers( _device, &allocInfo, &commandBuffer);

	{
		VkCommandBufferBeginInfo beginInfo {
			.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
			.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
		};

		vkBeginCommandBuffer( commandBuffer, &beginInfo );

		VkBufferCopy copyRegion {
			.srcOffset = 0,
			.dstOffset = 0,
			.size = size,
		};

		vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

		vkEndCommandBuffer(commandBuffer);
	}

	VkSubmitInfo submitInfo {
		.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
		.commandBufferCount = 1,
		.pCommandBuffers = &commandBuffer
	};

	vkQueueSubmit( _graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE );
	vkQueueWaitIdle( _graphicsQueue );

	vkFreeCommandBuffers(_device, _commandPool, 1, &commandBuffer);
}
	
void VulkanEngine::recordCommandBuffer( VkCommandBuffer commandBuffer, uint imageIndex, int flightFrame ) {

	VkCommandBufferBeginInfo beginInfo {

		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
		.flags = 0,
		.pInheritanceInfo = nullptr
	};

	if ( vkBeginCommandBuffer( commandBuffer, &beginInfo ) != VK_SUCCESS ) {

		throw std::runtime_error("Failed to begin recording command buffer");
	}

	std::array<VkClearValue, 2> clearColors = { 
		{ { .6f, .6f, .6f, 1.f } },
	};

	clearColors[1].depthStencil = { 1.0f, 0 };

	VkRenderPassBeginInfo renderPassInfo {
		
		.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
		.renderPass = _renderPass,
		.framebuffer = _swapchainFramebuffers[imageIndex],
		.renderArea = {
			.offset = {0, 0},
			.extent = _swapchainExtent,
		},
		.clearValueCount = clearColors.size(),
		.pClearValues = clearColors.data()
	};

	vkCmdBeginRenderPass( commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE );

	vkCmdBindPipeline( commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, _mainGraphicsPipeline );

	VkViewport viewport{};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = static_cast<float>(std::round(_swapchainExtent.width));
	viewport.height = static_cast<float>(std::round(_swapchainExtent.height));
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;

	vkCmdSetViewport( commandBuffer, 0, 1, &viewport );

	VkRect2D scissor{};
	scissor.offset = {0, 0};
	scissor.extent = _swapchainExtent;
	vkCmdSetScissor( commandBuffer, 0, 1, &scissor );

	vkCmdBindDescriptorSets( commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, _pipelineLayout, 
							0, 1, &_descriptorSets[flightFrame], 0, nullptr );

	{
		VkBuffer vertexBuffers[] = { _vertexBuffer };
		VkDeviceSize offsets[] = { 0 };
		vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets );
		vkCmdBindIndexBuffer( commandBuffer, _indexBuffer, 0, VK_INDEX_TYPE_UINT16 );

		vkCmdDrawIndexed( commandBuffer, _numberOfIndices, 1, 0, 0, 0 );
	}

	vkCmdEndRenderPass( commandBuffer );

	if ( vkEndCommandBuffer( commandBuffer ) != VK_SUCCESS ) {

		throw std::runtime_error("Failed to record command buffer");
	}
}

void VulkanEngine::createSyncObjects() {

	VkSemaphoreCreateInfo semaphoreInfo{
		.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
	};

	_imageAvailableSemaphores.resize( MAX_FRAMES_IN_FLIGHT );
	_renderFinishedSemaphores.resize( MAX_FRAMES_IN_FLIGHT );

	VkFenceCreateInfo fenceInfo{
		.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
		.flags = VK_FENCE_CREATE_SIGNALED_BIT
	};

	_inFlightFences.resize( MAX_FRAMES_IN_FLIGHT );

	for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {

		if ( vkCreateSemaphore( _device, &semaphoreInfo, nullptr, &_imageAvailableSemaphores[i] ) != VK_SUCCESS ||
			vkCreateSemaphore( _device, &semaphoreInfo, nullptr, &_renderFinishedSemaphores[i] ) != VK_SUCCESS ) {

			throw std::runtime_error("Unable to create semaphores");
		}

		if ( vkCreateFence( _device, &fenceInfo, nullptr, &_inFlightFences[i] ) != VK_SUCCESS ) {

			throw std::runtime_error("Unable to create fences");
		} 
	}
}

void VulkanEngine::createBuffer( 
		VkDeviceSize bufferSize, VkBufferUsageFlags usageFlags,
		VkMemoryPropertyFlags memProps, VkBuffer &buffer, VkDeviceMemory &bufferMemory ){

	{
		VkBufferCreateInfo createInfo {
			.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
			.size = bufferSize,
			.usage = usageFlags,
			.sharingMode = VK_SHARING_MODE_EXCLUSIVE
		};

		if ( vkCreateBuffer( _device, &createInfo, nullptr, &buffer) != VK_SUCCESS ) {

			throw std::runtime_error("Failed to create buffer");
		}
	}

	// Alloc memory and bind it to the buffer
	{
		VkMemoryRequirements memRequirements;
		vkGetBufferMemoryRequirements( _device, buffer, &memRequirements );

		VkMemoryAllocateInfo allocInfo {
			.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
			.allocationSize = memRequirements.size,
			.memoryTypeIndex = findMemoryType( memRequirements.memoryTypeBits, memProps )
		};

		if ( vkAllocateMemory( _device, &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS ) {

			throw std::runtime_error("Unable to allocate buffer memory");
		}

		vkBindBufferMemory( _device, buffer, bufferMemory, 0 );
	}
}

void VulkanEngine::createDescriptorSetlayout() {
	
	VkDescriptorSetLayoutBinding uboLayoutBinding {
		.binding = 0,
		.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
		.descriptorCount = 1,
		.stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
		.pImmutableSamplers = nullptr
	};

	VkDescriptorSetLayoutBinding samplerLayoutBinding {
		.binding = 1,
		.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
		.descriptorCount = 1,
		.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
		.pImmutableSamplers = nullptr,
	};

	std::array<VkDescriptorSetLayoutBinding, 2> bindings = { uboLayoutBinding, samplerLayoutBinding };

	VkDescriptorSetLayoutCreateInfo layoutInfo {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
		.bindingCount = bindings.size(),
		.pBindings = bindings.data()
	};

	if ( vkCreateDescriptorSetLayout( _device, &layoutInfo, nullptr, &_descriptorSetLayout ) != VK_SUCCESS ) {

		throw std::runtime_error("Failed to create descriptor layout");
	}
}

void VulkanEngine::createUniformBuffer() {
	VkDeviceSize bufferSize = sizeof( UniformBufferObject );

	for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {

		VkBuffer buffer;
		VkDeviceMemory memory;

		createBuffer( bufferSize, 
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, 
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, 
			buffer, memory);

		_uniformBuffers.push_back( buffer );
		_uniformBufferMemory.push_back( memory );

		void* mappedMemory;

		vkMapMemory( _device, memory, 0, bufferSize, 0, &mappedMemory);

		_uniformBufferMapped.push_back( mappedMemory );
	}
}

void VulkanEngine::updateUniformBuffer( int flightFrame ) {

	static auto startTime = std::chrono::high_resolution_clock::now();

	auto currentTime = std::chrono::high_resolution_clock::now();
	float elapsedTimeSinceStartup = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

	glm::vec3 eyePos = glm::vec3(1.0f, 1.0f, 1.0f) * (24 + abs(sin(elapsedTimeSinceStartup)) * 4);
	glm::vec3 targetPos = glm::vec3(0.0f, 0.0f, 12.0f);
	glm::vec3 upVec = glm::vec3(0, 0, -1);

	UniformBufferObject ubo{};
	ubo.model = glm::rotate( glm::mat4(1.0f), glm::radians( 180.0f ), glm::vec3( 0.0f, 0.0f, 1.0f) );
	ubo.view = glm::lookAt(eyePos, targetPos, upVec);
	ubo.proj = glm::perspective(glm::radians(45.0f), _swapchainExtent.width / (float) _swapchainExtent.height, 0.1f, 1000.0f);

	memcpy( _uniformBufferMapped[flightFrame], &ubo, sizeof(ubo) );
}

void VulkanEngine::createDescriptorPool() {

	std::array<VkDescriptorPoolSize, 2> poolSize {
		VkDescriptorPoolSize {
			.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
			.descriptorCount = MAX_FRAMES_IN_FLIGHT
		},
		VkDescriptorPoolSize {
			.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			.descriptorCount = MAX_FRAMES_IN_FLIGHT
		},
	};

	VkDescriptorPoolCreateInfo createInfo {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
		.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT,
		.maxSets = MAX_FRAMES_IN_FLIGHT,
		.poolSizeCount = poolSize.size(),
		.pPoolSizes = poolSize.data(),
	};

	if ( vkCreateDescriptorPool( _device, &createInfo, nullptr, &_descriptorPool ) != VK_SUCCESS ) {

		throw std::runtime_error( "Failed to create descriptor pool" );
	}
}

void VulkanEngine::allocDescriptorSets() {

	vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, _descriptorSetLayout);

	VkDescriptorSetAllocateInfo allocInfo {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
		.descriptorPool = _descriptorPool,
		.descriptorSetCount = MAX_FRAMES_IN_FLIGHT,
		.pSetLayouts = layouts.data(),
	};

	_descriptorSets.resize( MAX_FRAMES_IN_FLIGHT );

	if ( vkAllocateDescriptorSets( _device, &allocInfo, _descriptorSets.data() ) != VK_SUCCESS ) {

		throw std::runtime_error( "Failed to allocate descriptor set" );
	}

	for( int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++ ) {

		VkDescriptorBufferInfo bufferInfo {
			.buffer = _uniformBuffers[i],
			.offset = 0,
			.range = sizeof(UniformBufferObject)
		};

		VkDescriptorImageInfo imageInfo {
			.sampler = _textureSampler,
			.imageView = _textureImageView,
			.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
		};

		std::array<VkWriteDescriptorSet, 2> descriptorWrites {

			VkWriteDescriptorSet {
				.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
				.dstSet = _descriptorSets[i],
				.dstBinding = 0,
				.dstArrayElement = 0,
				.descriptorCount = 1,
				.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
				.pImageInfo = nullptr,
				.pBufferInfo = &bufferInfo,
				.pTexelBufferView = nullptr,
			},

			VkWriteDescriptorSet {
				.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
				.dstSet = _descriptorSets[i],
				.dstBinding = 1,
				.dstArrayElement = 0,
				.descriptorCount = 1,
				.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
				.pImageInfo = &imageInfo,
				.pTexelBufferView = nullptr,
			}
		};

		vkUpdateDescriptorSets( _device, descriptorWrites.size(), descriptorWrites.data(), 0, nullptr );
	}
}

VkCommandBuffer VulkanEngine::beginSingleTimeCommands() {

	VkCommandBufferAllocateInfo allocInfo {
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
		.commandPool = _commandPool,
		.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
		.commandBufferCount = 1,
	};

	VkCommandBuffer allocatedBuffer;
	vkAllocateCommandBuffers( _device, &allocInfo, &allocatedBuffer );

	VkCommandBufferBeginInfo beginInfo {
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
		.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
	};

	vkBeginCommandBuffer( allocatedBuffer, &beginInfo );

	return allocatedBuffer;
}

void VulkanEngine::endSingleTimeCommands( VkCommandBuffer commandBuffer ) {

	vkEndCommandBuffer( commandBuffer );

	VkSubmitInfo submitInfo {
		.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
		.commandBufferCount = 1,
		.pCommandBuffers = &commandBuffer
	};

	vkQueueSubmit( _graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE );
	vkQueueWaitIdle( _graphicsQueue );

	vkFreeCommandBuffers( _device, _commandPool, 1, &commandBuffer );
}

void VulkanEngine::createImage(uint width, uint height, ImageParams parameters, VkImage& image, VkDeviceMemory& imageMemory) {

	// TODO: optimize for release
	parameters.validate();

	VkImageCreateInfo imageInfo{
		.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
		.flags = 0,
		.imageType = VK_IMAGE_TYPE_2D,
		.format = parameters.optFormat.value(),
		.extent = {.width = static_cast<uint>(width),
					.height = static_cast<uint>(height),
					.depth = 1},
		.mipLevels = 1,
		.arrayLayers = 1,
		.samples = VK_SAMPLE_COUNT_1_BIT,
		.tiling = parameters.optTiling.value(),
		.usage = parameters.optUsageFlags.value(),
		.sharingMode = VK_SHARING_MODE_EXCLUSIVE,
		.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
	};

	if (vkCreateImage(_device, &imageInfo, nullptr, &image) !=
		VK_SUCCESS) {

	throw std::runtime_error("Failed to create texture image");
	}

	VkMemoryRequirements memRequirements;
	vkGetImageMemoryRequirements(_device, image, &memRequirements);

	VkMemoryAllocateInfo allocInfo {
	.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
	.allocationSize = memRequirements.size,
	.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits,
										VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)
	};

	if ( vkAllocateMemory( _device, &allocInfo, nullptr, &imageMemory ) !=
		VK_SUCCESS) {

	throw std::runtime_error("Failed to allocate memory for image");
	}

	vkBindImageMemory( _device, image, imageMemory, 0 );
}

void VulkanEngine::createTextureImage( Image image ) {

	VkDeviceSize imageSize = image.getSize();

	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;

	createBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
					VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
				stagingBuffer, stagingBufferMemory);

	void *data;

	vkMapMemory(_device, stagingBufferMemory, 0, imageSize, 0, &data);
	memcpy(data, image.getPixelPointer(), image.getSize());
	vkUnmapMemory(_device, stagingBufferMemory);

	const auto format = VK_FORMAT_R8G8B8A8_SRGB;
	const auto imageParameters = samplerImageParams.Overriden( 
		{ .optFormat = format } 
	);

	createImage( image.getWidth(), image.getHeight(), imageParameters, _textureImage, _textureImageMemory );
	transitionImageLayout( _textureImage, format, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL );
	copyBufferToImage( stagingBuffer, _textureImage, image.getWidth(), image.getHeight() );
	transitionImageLayout( _textureImage, format, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL );

	vkDestroyBuffer( _device, stagingBuffer, nullptr );
	vkFreeMemory( _device, stagingBufferMemory, nullptr );
}

void VulkanEngine::createTextureImageView() {

	if ( createImageView( _textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT, &_textureImageView ) != VK_SUCCESS ) {

		throw std::runtime_error( "Failed to create texture image view" );
	}

}

VkResult VulkanEngine::createImageView( VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, VkImageView* pView ) {

	VkImageViewCreateInfo viewInfo {
		.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
		.image = image,
		.viewType = VK_IMAGE_VIEW_TYPE_2D,
		.format = format,
		.components {
			.r = VK_COMPONENT_SWIZZLE_IDENTITY,
			.g = VK_COMPONENT_SWIZZLE_IDENTITY,
			.b = VK_COMPONENT_SWIZZLE_IDENTITY,
			.a = VK_COMPONENT_SWIZZLE_IDENTITY,
		},
		.subresourceRange = {
			.aspectMask = aspectFlags,
			.baseMipLevel = 0,
			.levelCount = 1,
			.baseArrayLayer = 0,
			.layerCount = 1,
		},

	};

	return vkCreateImageView( _device, &viewInfo, nullptr, pView );
}

void VulkanEngine::transitionImageLayout( VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout ) {

	VkCommandBuffer commandBuffer = beginSingleTimeCommands();

	VkPipelineStageFlags sourceStage;
	VkPipelineStageFlags destinationStage;
	VkAccessFlags sourceAccessMask;
	VkAccessFlags destinationAccessMask;

	bool hasDepthAttachment = newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	if ( oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL ) {

		sourceAccessMask = 0;
		destinationAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

		sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
	
	} else if ( oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL ) {

		sourceAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		destinationAccessMask = VK_ACCESS_SHADER_READ_BIT;

		sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;

	} else if ( oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && hasDepthAttachment ) {

		sourceAccessMask = 0;
		destinationAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

		sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		destinationStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;

	} else {

		throw std::runtime_error("Unsupported layout transition.");
	}

	VkImageMemoryBarrier barrier {
		.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
		.srcAccessMask = sourceAccessMask,
		.dstAccessMask = destinationAccessMask,
		.oldLayout = oldLayout,
		.newLayout = newLayout,
		.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
		.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
		.image = image,
		.subresourceRange = {
			.aspectMask = hasDepthAttachment ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT,
			.baseMipLevel = 0,
			.levelCount = 1,
			.baseArrayLayer = 0,
			.layerCount = 1
		},
	};

	vkCmdPipelineBarrier( commandBuffer, sourceStage, destinationStage, 0, 
							0, nullptr, 
							0, nullptr, 
							1, &barrier );

	endSingleTimeCommands( commandBuffer );
}

void VulkanEngine::createTextureSampler() {

	VkPhysicalDeviceProperties properties {};
	vkGetPhysicalDeviceProperties( _physicalDevice, &properties );

	VkSamplerCreateInfo samplerInfo {
		.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
		.magFilter = VK_FILTER_LINEAR,
		.minFilter = VK_FILTER_LINEAR,
		.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR,
		.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, // TODO: organize properly, rearrange with address mode
		.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
		.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
		.mipLodBias = 0.0f,
		.anisotropyEnable = VK_TRUE,
		.maxAnisotropy = properties.limits.maxSamplerAnisotropy,
		.compareEnable = VK_FALSE,
		.compareOp = VK_COMPARE_OP_ALWAYS,
		.minLod = 0.0f,
		.maxLod = 0.0f,
		.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE,
		.unnormalizedCoordinates = VK_FALSE,
	};

	if ( vkCreateSampler( _device, &samplerInfo, nullptr, &_textureSampler ) != VK_SUCCESS ) {

		throw std::runtime_error("Failed to create texture sampler!");
	}
}

void VulkanEngine::copyBufferToImage( VkBuffer buffer, VkImage image, uint width, uint height) {

	VkCommandBuffer commandBuffer = beginSingleTimeCommands();

	VkBufferImageCopy region {
		.bufferOffset = 0,
		.bufferRowLength = 0,
		.bufferImageHeight = 0,
		.imageSubresource = {
			.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
			.mipLevel = 0,
			.baseArrayLayer = 0,
			.layerCount = 1,
		},
		.imageOffset = { 0,0,0 },
		.imageExtent = {
			width, height, 1
		}
	};

	vkCmdCopyBufferToImage( commandBuffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region );

	endSingleTimeCommands( commandBuffer );
}

void VulkanEngine::createDepthResources() {

	VkFormat depthFormat = findDepthFormat();
	const auto imageParameters = samplerImageParams.Overriden( {
		.optFormat = depthFormat,
		.optUsageFlags = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT
	});

	createImage( _swapchainExtent.width, _swapchainExtent.height, imageParameters, _depthImage, _depthImageMemory );
	createImageView( _depthImage, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT, &_depthImageView );

	transitionImageLayout( _depthImage, depthFormat, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL );
}

VkFormat VulkanEngine::findDepthFormat() {

	auto candidateFormats = { VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT };

	return findSupportedFormat( candidateFormats, VK_IMAGE_TILING_OPTIMAL, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT );
}

VkFormat VulkanEngine::findSupportedFormat(
	const vector<VkFormat> &candidates, VkImageTiling tiling, VkFormatFeatureFlags features) {

	for ( VkFormat format : candidates ) {

		VkFormatProperties props;
		vkGetPhysicalDeviceFormatProperties( _physicalDevice, format, &props );

		if ( tiling == VK_IMAGE_TILING_LINEAR && 
				( props.linearTilingFeatures & features ) == features ) {

			return format;
		} else if ( tiling == VK_IMAGE_TILING_OPTIMAL && 
				(props.optimalTilingFeatures & features) == features ) {

			return format;
		}
	}

	throw std::runtime_error("Failed to find supported format!");
}

void VulkanEngine::drawFrame() {

	// Wait for previous frame to be rendered
	int flightFrame = _currentFrame++ % MAX_FRAMES_IN_FLIGHT;

	vkWaitForFences( _device, 1, &_inFlightFences[flightFrame], VK_TRUE, UINT64_MAX );
	vkResetFences( _device, 1, &_inFlightFences[flightFrame] );

	uint imageIndex;

	vkAcquireNextImageKHR( _device, _swapchain, UINT64_MAX, 
							_imageAvailableSemaphores[flightFrame], VK_NULL_HANDLE, &imageIndex );

	// Record new commands
	updateUniformBuffer( flightFrame );
	vkResetCommandBuffer( _commandBuffers[flightFrame], 0 );
	recordCommandBuffer( _commandBuffers[flightFrame], imageIndex, flightFrame );

	// Submit command buffer to queue
	VkSemaphore waitSemaphore[] = { _imageAvailableSemaphores[flightFrame] };
	VkSemaphore signalSemaphores[] { _renderFinishedSemaphores[flightFrame] };
	VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };

	VkSubmitInfo submitInfo { 
		.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
		.waitSemaphoreCount = 1,
		.pWaitSemaphores = waitSemaphore,
		.pWaitDstStageMask = waitStages,
		.commandBufferCount = 1,
		.pCommandBuffers = &_commandBuffers[flightFrame],
		.signalSemaphoreCount = 1,
		.pSignalSemaphores = signalSemaphores,
	};

	if ( vkQueueSubmit( _graphicsQueue, 1, &submitInfo, _inFlightFences[flightFrame] ) != VK_SUCCESS ) {

		throw std::runtime_error("Failed to submit draw command buffer");
	}

	VkSwapchainKHR swapChains[] = { _swapchain };
	VkPresentInfoKHR presentInfo { 
		.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
		.waitSemaphoreCount = 1,
		.pWaitSemaphores = signalSemaphores,
		.swapchainCount = 1,
		.pSwapchains = swapChains,
		.pImageIndices = &imageIndex,
		.pResults = nullptr,
	};

	vkQueuePresentKHR( _presentQueue, &presentInfo );
}

void VulkanEngine::deviceWaitIdle() {

	vkDeviceWaitIdle( _device );
}

void VulkanEngine::release() {

	vkDestroyImageView( _device, _depthImageView, nullptr );
	vkDestroyImage( _device, _depthImage, nullptr );
	vkFreeMemory( _device, _depthImageMemory, nullptr );

	vkDestroySampler( _device, _textureSampler, nullptr );

	vkDestroyImageView( _device, _textureImageView, nullptr );

	vkDestroyImage( _device, _textureImage, nullptr );
	vkFreeMemory( _device, _textureImageMemory, nullptr );

	for( int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++ ) {

		vkFreeDescriptorSets( _device, _descriptorPool, 1, &_descriptorSets[i] );
	}

	_descriptorSets.clear();

	vkDestroyDescriptorPool( _device, _descriptorPool, nullptr );
	_descriptorPool = nullptr;

	for ( int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++ ) {

		vkDestroyBuffer( _device, _uniformBuffers[i], nullptr );
		vkFreeMemory( _device, _uniformBufferMemory[i], nullptr );

	}

	_uniformBuffers.clear();
	_uniformBufferMemory.clear();
	_uniformBufferMapped.clear();

	vkDestroyDescriptorSetLayout( _device, _descriptorSetLayout, nullptr);
	_descriptorSetLayout = nullptr;

	vkDestroyBuffer( _device, _indexBuffer, nullptr );
	_indexBuffer = nullptr;

	vkFreeMemory( _device, _indexBufferMemory, nullptr );
	_indexBufferMemory = nullptr;

	vkDestroyBuffer( _device, _vertexBuffer, nullptr );
	_vertexBuffer = nullptr;

	vkFreeMemory( _device, _vertexBufferMemory, nullptr );
	_vertexBufferMemory = nullptr;

	for ( int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++ ) {

		vkDestroySemaphore( _device, _imageAvailableSemaphores[i], nullptr );
		vkDestroySemaphore( _device, _renderFinishedSemaphores[i], nullptr );
		vkDestroyFence( _device, _inFlightFences[i], nullptr );
	}

	_imageAvailableSemaphores.clear();
	_renderFinishedSemaphores.clear();
	_inFlightFences.clear();

	vkDestroyCommandPool( _device, _commandPool, nullptr );
	_commandPool = nullptr;

	for(auto framebuffer : _swapchainFramebuffers) {

		vkDestroyFramebuffer( _device, framebuffer, nullptr );
	}

	_swapchainFramebuffers.clear();

	vkDestroyPipeline( _device, _mainGraphicsPipeline, nullptr );
	_mainGraphicsPipeline = nullptr;

	vkDestroyPipelineLayout( _device, _pipelineLayout, nullptr );
	_pipelineLayout = nullptr;

	vkDestroyRenderPass( _device, _renderPass, nullptr );
	_renderPass = nullptr;

	_mainShader.release();

	for(auto imageView : _swapchainImageViews) {

		vkDestroyImageView( _device, imageView, nullptr );
	}

	_swapchainImageViews.clear();

	vkDestroySwapchainKHR( _device, _swapchain, nullptr );
	_swapchain = nullptr;

	vkDestroySurfaceKHR( _instance, _surface, nullptr );
	_surface = nullptr;

	vkDestroyDevice( _device, nullptr );
	_device = nullptr;

	vkDestroyInstance( _instance, nullptr );
	_instance = nullptr;

	_safe = false;
}

bool VulkanEngine::isSafe()
{
	return _safe;
}
