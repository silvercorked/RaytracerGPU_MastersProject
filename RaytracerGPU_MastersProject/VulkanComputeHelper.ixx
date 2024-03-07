module;

#include <iostream>
#include <fstream>
#include <unordered_set>
#include <set>
#include <array>
#include <random>
#include <chrono>

//#include <vulkan/vulkan.h>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS					// functions expect radians, not degrees
#define GLM_FORCE_DEPTH_ZERO_TO_ONE			// Depth buffer values will range from 0 to 1, not -1 to 1
#include <glm/glm.hpp>

export module VulkanComputeHelper;

import PrimitiveTypes;
import Bitmap;

import VulkanWrap;

constexpr const u32 SAMPLE_COUNT = 1024*512;
constexpr const u32 KERNEL_SIZE = 1024; // needs to be synced with shader x value

struct UniformBufferObject {
	glm::vec4 pixelColor;
	float iteration;
	float width;
	float height;
};

struct Logistic { // f(x; r) = xr(1-x)
	float x; // 1 ssbo is used to store all these.
	float r;
};

export class VulkanComputeHelper {
	// createInstance
	//VkInstance instance;
	//const std::vector<const char*> validationLayers = { "VK_LAYER_KHRONOS_validation" };
	
	// setupDebugMessenger
	//VkDebugUtilsMessengerEXT debugMessenger;

	// createSurface
	Window window;
	//VkSurfaceKHR surface;

	// pickPhysicalDevice
	//VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
	VkPhysicalDeviceProperties properties;

	// createLogicalDevice
	Device device;
	//VkDevice device;
	//const std::vector<const char*> deviceExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };
	//u32 GCQueueIndex;
	//u32 PresentQueueIndex;
	//VkQueue GCQueue; // graphics and compute queue
	//VkQueue PresentQueue;

	// createSwapChain
	u32 currentSwapChainIndex = 0;
	VkSwapchainKHR swapChain;
	std::vector<VkImage> swapChainImages;
	VkFormat swapChainImageFormat;
	VkExtent2D swapChainExtent;
	
	// createImageViews
	std::vector<VkImageView> swapChainImageViews;

	// createRenderPass
	VkRenderPass renderPass;

	// createComputeDescriptorSetLayout
	VkDescriptorSetLayout computeDescriptorSetLayout;

	// createComputePipeline
	VkShaderModule computeShaderModule;
	VkPipelineShaderStageCreateInfo computeShaderStageInfo;
	VkPipeline computePipeline;
	VkPipelineLayout computePipelineLayout;

	// createFrameBuffers
	std::vector<VkFramebuffer> swapChainFrameBuffers;

	// createCommandPool
	//VkCommandPool commandPool;

	// createShaderStorageBuffers
	std::vector<VkBuffer> shaderStorageBuffers;				// strictly 2 for this case. no swapchain so just the ones i need
	std::vector<VkDeviceMemory> shaderStorageBuffersMemory;	// which are the prev and current

	// createUniformBuffers
	std::unique_ptr<Buffer> uniformBuffer;

	// createDesciptorPool
	VkDescriptorPool descriptorPool;

	// createComputeDescriptorSets
	std::vector<VkDescriptorSet> computeDescriptorSets;

	// createComputeCommandBuffers
	std::vector<VkCommandBuffer> computeCommandBuffers;
	std::vector<VkCommandBuffer> transferCommandBuffers;

	// createSyncObjects
	VkSemaphore computeFinishedSemaphore;
	VkFence computeInFlightFence;
	VkSemaphore transferDoneSemaphore;
	VkSemaphore imageReadySemaphore;

	// mainLoop -> doIteration
	u32 iteration;

	auto initVulkan() -> void {
		/* Device
			this->createInstance();
			this->setupDebugMessenger();
			this->createSurface();
			this->pickPhysicalDevice();
			this->createLogicalDevice();
			this->createCommandPool();
		*/
		this->createSwapChain();
		this->createImageViews();				// out image
		this->createRenderPass();
		this->createComputeDescriptorSetLayout();
		this->createComputePipeline();
		this->createFrameBuffers();
		
		this->createUniformBuffers();			// in ubo
		this->createShaderStorageBuffers();		// in ssbo
		this->createDescriptorPool();
		this->createComputeDescriptorSets();
		this->createComputeCommandBuffers();
		this->createSyncObjects();
	}

	auto createSwapChain() -> void;
	auto chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats) -> VkSurfaceFormatKHR;
	auto chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes) -> VkPresentModeKHR;
	auto chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities) -> VkExtent2D;

	auto createImageViews() -> void;

	auto createRenderPass() -> void;

	auto createComputeDescriptorSetLayout() -> void;

	auto createComputePipeline() -> void;
	auto createShaderModule(const std::vector<char>&) -> VkShaderModule;
	auto readFile(const std::string&) -> std::vector<char>;

	auto createFrameBuffers() -> void;

	auto createShaderStorageBuffers() -> void;
	auto createBuffer(
		VkDeviceSize,
		VkBufferUsageFlags,
		VkMemoryPropertyFlags,
		VkBuffer&,
		VkDeviceMemory&
	) -> void;
	auto copyBuffer(VkQueue, VkCommandPool, VkBuffer, VkBuffer, VkDeviceSize) -> void;

	auto createUniformBuffers() -> void;

	auto createDescriptorPool() -> void;

	auto createComputeDescriptorSets() -> void;

	auto createComputeCommandBuffers() -> void;

	auto createSyncObjects() -> void;

	auto doIteration() -> void {
		// fences are for syncing cpu and gpu. semaphores are for specifying the order of gpu tasks
		u32 nextSwapChainIndex = this->getNextImageIndex(); // signals imageReadySemaphore
		vkResetFences(this->device.device(), 1, &this->computeInFlightFence); // reset for next run

		UniformBufferObject nUbo{};
		nUbo.iteration = this->iteration;
		nUbo.pixelColor = glm::vec4(1.0, 0.0, 0.0, 1.0);
		nUbo.width = static_cast<f32>(this->swapChainExtent.width);
		nUbo.height = static_cast<f32>(this->swapChainExtent.height);
		this->uniformBuffer->writeToBuffer(&nUbo);
		this->uniformBuffer->flush(); // make visible to device
		std::cout << "doing iteration" << std::endl;

		VkPipelineStageFlags computeStage = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
		VkPipelineStageFlags transferStage = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;

		VkSubmitInfo submitInfo{};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &this->computeCommandBuffers[nextSwapChainIndex];
		submitInfo.signalSemaphoreCount = 1;
		submitInfo.pSignalSemaphores = &this->computeFinishedSemaphore;
		submitInfo.waitSemaphoreCount = 1;
		submitInfo.pWaitSemaphores = &this->imageReadySemaphore;
		submitInfo.pWaitDstStageMask = &computeStage;

		if (vkQueueSubmit(this->device.computeQueue(), 1, &submitInfo, this->computeInFlightFence) != VK_SUCCESS) {
			throw std::runtime_error("failed to submit compute command buffer!");
		}
		this->iteration++;

		vkWaitForFences(this->device.device(), 1, &this->computeInFlightFence, VK_TRUE, UINT64_MAX); // await compute completion

		 { // fake 1 second delay
			auto currentTime = std::chrono::high_resolution_clock::now();
			auto newTime = std::chrono::high_resolution_clock::now();
			while (std::chrono::duration<float, std::chrono::milliseconds::period>(newTime - currentTime).count() < 250.0f)
				newTime = std::chrono::high_resolution_clock::now();
			std::cout << "waited 1 second probably" << std::endl;
		}

		this->presentToWindow(nextSwapChainIndex, this->computeFinishedSemaphore);

		//vkResetCommandBuffer(this->computeCommandBuffer, 0);
	}

	auto recordComputeCommandBuffer(VkCommandBuffer, u32) -> void;

	auto getNextImageIndex() -> u32;

	auto presentToWindow(u32 swapChainImageIndex, VkSemaphore toWaitOn) -> void;

public:
	VulkanComputeHelper();
	auto mainLoop() -> void {
		auto currentTime = std::chrono::high_resolution_clock::now();

		for (size_t i = 0; i < this->swapChainImageViews.size(); i++) {
			this->recordComputeCommandBuffer(this->computeCommandBuffers[i], i);
		}

		while (!this->window.shouldClose()) {
			glfwPollEvents();
			//if (this->iteration > 0) break;
			auto newTime = std::chrono::high_resolution_clock::now();
			float frameTime = std::chrono::duration<float, std::chrono::milliseconds::period>(newTime - currentTime).count();
			currentTime = newTime;
			std::cout << "Iteration Time: " << frameTime << std::endl;
			doIteration();
		}
		vkDeviceWaitIdle(this->device.device());
	}
	~VulkanComputeHelper();
};

VulkanComputeHelper::VulkanComputeHelper() :
	window{1920, 1080, "Compute-based Images"},
	device{window}
{
	this->initVulkan();
}
VulkanComputeHelper::~VulkanComputeHelper() {
	vkDestroyPipeline(this->device.device(), computePipeline, nullptr);
	vkDestroyPipelineLayout(this->device.device(), computePipelineLayout, nullptr);

	this->uniformBuffer = nullptr; // deconstruct uniformBuffer

	vkDestroyDescriptorPool(this->device.device(), this->descriptorPool, nullptr);
	vkDestroyDescriptorSetLayout(this->device.device(), this->computeDescriptorSetLayout, nullptr);

	for (size_t i = 0; i < this->shaderStorageBuffers.size(); i++) {
		vkDestroyBuffer(this->device.device(), this->shaderStorageBuffers[i], nullptr);
		vkFreeMemory(this->device.device(), this->shaderStorageBuffersMemory[i], nullptr);
	}

	vkDestroySemaphore(this->device.device(), this->computeFinishedSemaphore, nullptr);
	vkDestroyFence(this->device.device(), this->computeInFlightFence, nullptr);

	/* ~Device
		vkDestroyCommandPool(this->device, this->commandPool, nullptr);
		vkDestroyDevice(this->device, nullptr);
		this->DestroyDebugUtilsMessengerEXT(this->instance, this->debugMessenger, nullptr);
		vkDestroyInstance(this->instance, nullptr);
	*/
}

auto VulkanComputeHelper::createSwapChain() -> void {
	SwapChainSupportDetails swapChainSupport = this->device.getSwapChainSupport();

	VkSurfaceFormatKHR surfaceFormat = this->chooseSwapSurfaceFormat(swapChainSupport.formats);
	VkPresentModeKHR presentMode = this->chooseSwapPresentMode(swapChainSupport.presentModes);
	VkExtent2D extent = this->chooseSwapExtent(swapChainSupport.capabilities);

	u32 imageCount = swapChainSupport.capabilities.minImageCount + 1;
	if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount) {
		imageCount = swapChainSupport.capabilities.maxImageCount;
	}

	VkSwapchainCreateInfoKHR createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	createInfo.surface = this->device.surface();
	createInfo.minImageCount = imageCount;
	createInfo.imageFormat = surfaceFormat.format;
	createInfo.imageColorSpace = surfaceFormat.colorSpace;
	createInfo.imageExtent = extent;
	createInfo.imageArrayLayers = 1;
	createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_STORAGE_BIT;

	QueueFamilyIndices indices = this->device.findPhysicalQueueFamilies();
	u32 queueFamilyIndices[] = { indices.graphicsFamily, indices.presentFamily };

	if (indices.graphicsFamily != indices.presentFamily) {
		createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		createInfo.queueFamilyIndexCount = 2;
		createInfo.pQueueFamilyIndices = queueFamilyIndices;
	}
	else {
		createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
	}

	createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
	createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	createInfo.presentMode = presentMode;
	createInfo.clipped = VK_TRUE;

	if (vkCreateSwapchainKHR(this->device.device(), &createInfo, nullptr, &this->swapChain) != VK_SUCCESS) {
		throw std::runtime_error("failed to create swap chain");
	}

	vkGetSwapchainImagesKHR(this->device.device(), this->swapChain, &imageCount, nullptr);
	swapChainImages.resize(imageCount);
	vkGetSwapchainImagesKHR(this->device.device(), this->swapChain, &imageCount, swapChainImages.data());
	swapChainImageFormat = surfaceFormat.format;
	swapChainExtent = extent;
}
auto VulkanComputeHelper::chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats) -> VkSurfaceFormatKHR {
	/*
	validation layer: Validation Error: [ VUID-VkSwapchainCreateInfoKHR-imageFormat-01778 ] Object 0: handle = 0x20a4db04db0, type = VK_OBJECT_TYPE_DEVICE; | MessageID = 0xc036022f | vkCreateSwapchainKHR(): pCreateInfo->imageFormat VK_FORMAT_B8G8R8A8_SRGB with tiling VK_IMAGE_TILING_OPTIMAL does not support usage that includes VK_IMAGE_USAGE_STORAGE_BIT. The Vulkan spec states: The implied image creation parameters of the swapchain must be supported as reported by vkGetPhysicalDeviceImageFormatProperties (https://vulkan.lunarg.com/doc/view/1.3.250.1/windows/1.3-extensions/vkspec.html#VUID-VkSwapchainCreateInfoKHR-imageFormat-01778)
	^^ this is why VK_FORMAT_B8G8R8A8_SRGB was replaced with VK_FORMAT_B8G8R8A8_UNORM
	*/
	for (const auto& availableFormat : availableFormats) {
		if (availableFormat.format == /*VK_FORMAT_B8G8R8A8_SRGB*/ VK_FORMAT_B8G8R8A8_UNORM && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
			return availableFormat;
		}
	}
	return availableFormats[0]; // default if desired aint there.
}

auto VulkanComputeHelper::chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes) -> VkPresentModeKHR {
	for (const auto& availablePresentMode : availablePresentModes) {
		if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
			return availablePresentMode;
		}
	}
	return VK_PRESENT_MODE_FIFO_KHR;
}
auto VulkanComputeHelper::chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities) -> VkExtent2D {
	if (capabilities.currentExtent.width != std::numeric_limits<u32>::max()) {
		return capabilities.currentExtent;
	}
	else {
		int width, height;
		glfwGetFramebufferSize(this->window.window(), &width, &height);
		VkExtent2D actualExtent = {
			static_cast<u32>(width),
			static_cast<u32>(height)
		};
		actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
		actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);
		return actualExtent;
	}
}

auto VulkanComputeHelper::createImageViews() -> void {
	this->swapChainImageViews.resize(this->swapChainImages.size());

	for (size_t i = 0; i < this->swapChainImages.size(); i++) {
		VkImageViewCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		createInfo.image = this->swapChainImages[i];
		createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		createInfo.format = swapChainImageFormat;
		createInfo.components.r = VK_COMPONENT_SWIZZLE_R;
		createInfo.components.g = VK_COMPONENT_SWIZZLE_G;
		createInfo.components.b = VK_COMPONENT_SWIZZLE_B;
		createInfo.components.a = VK_COMPONENT_SWIZZLE_A;
		createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		createInfo.subresourceRange.baseMipLevel = 0;
		createInfo.subresourceRange.levelCount = 1;
		createInfo.subresourceRange.baseArrayLayer = 0;
		createInfo.subresourceRange.layerCount = 1;

		if (vkCreateImageView(this->device.device(), &createInfo, nullptr, &this->swapChainImageViews[i]) != VK_SUCCESS) {
			throw std::runtime_error("failed to create image views!");
		}
	}
}

auto VulkanComputeHelper::createRenderPass() -> void {
	VkAttachmentDescription colorAttachment{};
	colorAttachment.format = this->swapChainImageFormat;
	colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	colorAttachment.initialLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	VkAttachmentReference colorAttachmentRef{};
	colorAttachmentRef.attachment = 0;
	colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpass{};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &colorAttachmentRef;

	VkSubpassDependency dependency{};
	dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	dependency.dstSubpass = 0;
	dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.srcAccessMask = 0;
	dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

	VkRenderPassCreateInfo renderPassInfo{};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassInfo.attachmentCount = 1;
	renderPassInfo.pAttachments = &colorAttachment;
	renderPassInfo.subpassCount = 1;
	renderPassInfo.pSubpasses = &subpass;
	renderPassInfo.dependencyCount = 1;
	renderPassInfo.pDependencies = &dependency;

	if (vkCreateRenderPass(this->device.device(), &renderPassInfo, nullptr, &this->renderPass) != VK_SUCCESS) {
		throw std::runtime_error("failed to create render pass!");
	}
}

auto VulkanComputeHelper::createComputeDescriptorSetLayout() -> void {
	std::array<VkDescriptorSetLayoutBinding, 3> layoutBindings{};
	layoutBindings[0].binding = 0;
	layoutBindings[0].descriptorCount = 1;
	layoutBindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	layoutBindings[0].pImmutableSamplers = nullptr;
	layoutBindings[0].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

	layoutBindings[1].binding = 1;
	layoutBindings[1].descriptorCount = 1;
	layoutBindings[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	layoutBindings[1].pImmutableSamplers = nullptr;
	layoutBindings[1].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

	layoutBindings[2].binding = 2;
	layoutBindings[2].descriptorCount = 1;
	layoutBindings[2].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
	layoutBindings[2].pImmutableSamplers = nullptr;
	layoutBindings[2].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

	VkDescriptorSetLayoutCreateInfo layoutInfo{};
	layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutInfo.bindingCount = 3;
	layoutInfo.pBindings = layoutBindings.data();

	if (vkCreateDescriptorSetLayout(this->device.device(), &layoutInfo, nullptr, &this->computeDescriptorSetLayout) != VK_SUCCESS) {
		throw std::runtime_error("failed to create compute descriptor set layout!");
	}
}

auto VulkanComputeHelper::createComputePipeline() -> void {
	auto computeShaderCode = readFile("shaders/compiled/logistic.comp.spv");

	this->computeShaderModule = this->createShaderModule(computeShaderCode);

	this->computeShaderStageInfo = {};
	this->computeShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	this->computeShaderStageInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
	this->computeShaderStageInfo.module = this->computeShaderModule;
	this->computeShaderStageInfo.pName = "main"; // function name to invoke in compute shader. main is prob good to always stick too. least for me rn

	VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = 1;
	pipelineLayoutInfo.pSetLayouts = &this->computeDescriptorSetLayout;

	if (vkCreatePipelineLayout(this->device.device(), &pipelineLayoutInfo, nullptr, &this->computePipelineLayout) != VK_SUCCESS)
		throw std::runtime_error("failed to create compute pipeline layout!");

	VkComputePipelineCreateInfo pipelineInfo{};
	pipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
	pipelineInfo.layout = this->computePipelineLayout;
	pipelineInfo.stage = this->computeShaderStageInfo;

	if (vkCreateComputePipelines(this->device.device(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &this->computePipeline) != VK_SUCCESS)
		throw std::runtime_error("failed to create compute pipeline!");

	vkDestroyShaderModule(this->device.device(), this->computeShaderModule, nullptr);
}
auto VulkanComputeHelper::createShaderModule(const std::vector<char>& code) -> VkShaderModule {
	VkShaderModuleCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	createInfo.codeSize = code.size();
	createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());
	VkShaderModule shaderModule;
	if (vkCreateShaderModule(this->device.device(), &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
		throw std::runtime_error("failed to create shader module!");
	}
	return shaderModule;
}
auto VulkanComputeHelper::readFile(const std::string& filepath) -> std::vector<char> { // needs <fstream>
	std::ifstream file{ filepath, std::ios::ate | std::ios::binary }; // jump to end and as binary
	if (!file.is_open()) {
		throw std::runtime_error("Failed to open file: " + filepath);
	}
	size_t fileSize = static_cast<size_t>(file.tellg());
	std::vector<char> buffer(fileSize);

	file.seekg(0);
	file.read(buffer.data(), fileSize);
	file.close();
	return buffer;
}

auto VulkanComputeHelper::createFrameBuffers() -> void {
	this->swapChainFrameBuffers.resize(this->swapChainImageViews.size());

	for (size_t i = 0; i < this->swapChainImageViews.size(); i++) {
		VkImageView attachments[] = {
			swapChainImageViews[i]
		};
		VkFramebufferCreateInfo framebufferInfo{};
		framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferInfo.renderPass = this->renderPass;
		framebufferInfo.attachmentCount = 1;
		framebufferInfo.pAttachments = attachments;
		framebufferInfo.width = this->swapChainExtent.width;
		framebufferInfo.height = this->swapChainExtent.height;
		framebufferInfo.layers = 1;

		if (vkCreateFramebuffer(this->device.device(), &framebufferInfo, nullptr, &this->swapChainFrameBuffers[i]) != VK_SUCCESS) {
			throw std::runtime_error("Failed to create frame buffer!");
		}
	}
}

auto VulkanComputeHelper::createShaderStorageBuffers() -> void {
	// initialize logistic sample points
	std::default_random_engine rndEngine((unsigned)time(nullptr));
	std::uniform_real_distribution<float> rndDist(0.0f, 1.0f); // parameter r is 0-4, x input is 0-1

	std::vector<Logistic> points(SAMPLE_COUNT);
	for (auto& point : points) {
		point.r = 3.5f + (rndDist(rndEngine) / 2.0f);			// r: 3.5-4
		point.x = rndDist(rndEngine);							// x: 0-1
	}

	VkDeviceSize bufferSize = sizeof(Logistic) * SAMPLE_COUNT;

	// Create Staging buffer to upload data to gpu
	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;
	this->createBuffer(
		bufferSize,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		stagingBuffer,
		stagingBufferMemory
	);

	void* data;
	vkMapMemory(this->device.device(), stagingBufferMemory, 0, bufferSize, 0, &data);
	memcpy(data, points.data(), static_cast<size_t>(bufferSize));
	vkUnmapMemory(this->device.device(), stagingBufferMemory);

	this->shaderStorageBuffers.resize(1);
	this->shaderStorageBuffersMemory.resize(1);

	this->createBuffer( // create ssbo buffers on the gpu that can be transfered to from the cpu
		bufferSize,
		VK_BUFFER_USAGE_STORAGE_BUFFER_BIT // is SSBO
		| VK_BUFFER_USAGE_TRANSFER_DST_BIT // can transfer to from cpu
		| VK_BUFFER_USAGE_TRANSFER_SRC_BIT, // can read from into cpu
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		this->shaderStorageBuffers[0],
		this->shaderStorageBuffersMemory[0]
	);
	this->copyBuffer(this->device.computeQueue(), this->device.getComputeCommandPool(), stagingBuffer, this->shaderStorageBuffers[0], bufferSize);

	vkDestroyBuffer(this->device.device(), stagingBuffer, nullptr);
	vkFreeMemory(this->device.device(), stagingBufferMemory, nullptr);
}
auto VulkanComputeHelper::createBuffer(
	VkDeviceSize size,
	VkBufferUsageFlags usage,
	VkMemoryPropertyFlags properties,
	VkBuffer& buffer,
	VkDeviceMemory& bufferMemory
) -> void {
	VkBufferCreateInfo bufferInfo{};
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.size = size;
	bufferInfo.usage = usage;
	bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	if (vkCreateBuffer(this->device.device(), &bufferInfo, nullptr, &buffer) != VK_SUCCESS)
		throw std::runtime_error("failed to create buffer!");

	VkMemoryRequirements memRequirements;
	vkGetBufferMemoryRequirements(this->device.device(), buffer, &memRequirements);

	VkMemoryAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memRequirements.size;
	allocInfo.memoryTypeIndex = this->device.findMemoryType(memRequirements.memoryTypeBits, properties);

	if (vkAllocateMemory(this->device.device(), &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS)
		throw std::runtime_error("failed to allocate buffer memory!");

	vkBindBufferMemory(this->device.device(), buffer, bufferMemory, 0);
}
auto VulkanComputeHelper::copyBuffer(VkQueue queue, VkCommandPool pool, VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size) -> void {
	VkCommandBufferAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandPool = pool;
	allocInfo.commandBufferCount = 1;

	VkCommandBuffer commandBuffer;
	vkAllocateCommandBuffers(this->device.device(), &allocInfo, &commandBuffer);

	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	vkBeginCommandBuffer(commandBuffer, &beginInfo);

	VkBufferCopy copyRegion{};
	copyRegion.size = size;
	vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

	vkEndCommandBuffer(commandBuffer);

	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffer;

	vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(queue);

    vkFreeCommandBuffers(this->device.device(), pool, 1, &commandBuffer);
}

auto VulkanComputeHelper::createUniformBuffers() -> void { // just 1
	VkDeviceSize bufferSize = sizeof(UniformBufferObject);
	this->uniformBuffer = std::make_unique<Buffer>(
		this->device,
		bufferSize,
		1,
		VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
	);
	this->uniformBuffer->map();
}

auto VulkanComputeHelper::createDescriptorPool() -> void {
	std::array<VkDescriptorPoolSize, 3> poolSizes{};
	poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	poolSizes[0].descriptorCount = static_cast<u32>(1);
	poolSizes[1].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	poolSizes[1].descriptorCount = static_cast<u32>(1);
	poolSizes[2].type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
	poolSizes[2].descriptorCount = static_cast<u32>(1);

	VkDescriptorPoolCreateInfo poolInfo{};
	poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolInfo.poolSizeCount = 3;
	poolInfo.pPoolSizes = poolSizes.data();
	poolInfo.maxSets = this->swapChainImageViews.size();

	if (vkCreateDescriptorPool(this->device.device(), &poolInfo, nullptr, &this->descriptorPool) != VK_SUCCESS)
		throw std::runtime_error("Failed to create descriptor pool!");
}

auto VulkanComputeHelper::createComputeDescriptorSets() -> void {
	std::vector<VkDescriptorSetLayout> layouts(this->swapChainImageViews.size(), this->computeDescriptorSetLayout);
	VkDescriptorSetAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = this->descriptorPool;
	allocInfo.descriptorSetCount = static_cast<u32>(this->swapChainImageViews.size());
	allocInfo.pSetLayouts = layouts.data();

	this->computeDescriptorSets.resize(this->swapChainImageViews.size());
	VkResult res = vkAllocateDescriptorSets(this->device.device(), &allocInfo, this->computeDescriptorSets.data());
	if (res != VK_SUCCESS) {
		throw std::runtime_error("Failed to allocate descriptor sets!");
	}

	for (size_t i = 0; i < this->swapChainImageViews.size(); i++) {
		VkDescriptorBufferInfo uboBufferInfo = this->uniformBuffer->descriptorInfo();
		std::array<VkWriteDescriptorSet, 3> descriptorWrites{};
		descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[0].dstSet = this->computeDescriptorSets[i];
		descriptorWrites[0].dstBinding = 0;
		descriptorWrites[0].dstArrayElement = 0;
		descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		descriptorWrites[0].descriptorCount = 1;
		descriptorWrites[0].pBufferInfo = &uboBufferInfo;

		VkDescriptorBufferInfo storageBufferInfo{};
		storageBufferInfo.buffer = this->shaderStorageBuffers[0];
		storageBufferInfo.offset = 0;
		storageBufferInfo.range = sizeof(Logistic) * SAMPLE_COUNT;

		descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[1].dstSet = this->computeDescriptorSets[i];
		descriptorWrites[1].dstBinding = 1;
		descriptorWrites[1].dstArrayElement = 0;
		descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		descriptorWrites[1].descriptorCount = 1;
		descriptorWrites[1].pBufferInfo = &storageBufferInfo;

		VkDescriptorImageInfo descImageInfo{};
		descImageInfo.sampler = nullptr;
		descImageInfo.imageView = this->swapChainImageViews[i];
		descImageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;

		descriptorWrites[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[2].dstSet = this->computeDescriptorSets[i];
		descriptorWrites[2].dstBinding = 2;
		descriptorWrites[2].dstArrayElement = 0;
		descriptorWrites[2].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
		descriptorWrites[2].descriptorCount = 1;
		descriptorWrites[2].pImageInfo = &descImageInfo;

		vkUpdateDescriptorSets(this->device.device(), 3, descriptorWrites.data(), 0, nullptr);
	}
}

auto VulkanComputeHelper::createComputeCommandBuffers() -> void {
	this->computeCommandBuffers.resize(this->swapChainImageViews.size());
	VkCommandBufferAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.commandPool = this->device.getComputeCommandPool();
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandBufferCount = static_cast<u32>(this->swapChainImageViews.size());

	if (vkAllocateCommandBuffers(this->device.device(), &allocInfo, this->computeCommandBuffers.data()) != VK_SUCCESS)
		throw std::runtime_error("Failed to allocate compute Command Buffers!");
}

auto VulkanComputeHelper::createSyncObjects() -> void {
	VkSemaphoreCreateInfo semaphoreInfo{};
	semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	VkFenceCreateInfo fenceInfo{};
	fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

	if (
		vkCreateSemaphore(this->device.device(), &semaphoreInfo, nullptr, &this->computeFinishedSemaphore) != VK_SUCCESS
		|| vkCreateFence(this->device.device(), &fenceInfo, nullptr, &computeInFlightFence) != VK_SUCCESS
		|| vkCreateSemaphore(this->device.device(), &semaphoreInfo, nullptr, &this->imageReadySemaphore) != VK_SUCCESS
		|| vkCreateSemaphore(this->device.device(), &semaphoreInfo, nullptr, &this->transferDoneSemaphore) != VK_SUCCESS
	)
		throw std::runtime_error("Failed to create compute synchronization object!");
}

void VulkanComputeHelper::recordComputeCommandBuffer(VkCommandBuffer commandBuffer, u32 nextSwapChainIndex) {
	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

	VkImageSubresourceRange range{};
	range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	range.levelCount = VK_REMAINING_MIP_LEVELS;
	range.layerCount = VK_REMAINING_ARRAY_LAYERS;
	range.baseArrayLayer = 0;
	range.baseMipLevel = 0;

	if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
		throw std::runtime_error("failed to begin recording compute command buffer!");
	}

	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, this->computePipeline);

	vkCmdBindDescriptorSets(
		commandBuffer,
		VK_PIPELINE_BIND_POINT_COMPUTE,
		this->computePipelineLayout,
		0,
		1,
		&this->computeDescriptorSets[nextSwapChainIndex],
		0,
		nullptr
	);

	VkClearColorValue clearValue = { 0.0f, 0.0f, 0.0f, 1.0f };
	vkCmdClearColorImage(
		commandBuffer, this->swapChainImages[nextSwapChainIndex],
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &clearValue, 1, &range
	);

	VkImageMemoryBarrier swapChainToCompute;
	swapChainToCompute.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	swapChainToCompute.pNext = nullptr;
	swapChainToCompute.srcAccessMask = 0;
	swapChainToCompute.dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
	swapChainToCompute.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	swapChainToCompute.newLayout = VK_IMAGE_LAYOUT_GENERAL;
	swapChainToCompute.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	swapChainToCompute.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	swapChainToCompute.image = this->swapChainImages[nextSwapChainIndex];
	swapChainToCompute.subresourceRange = range;

	vkCmdPipelineBarrier(
		commandBuffer,
		VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, // src stage
		VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, // dst stage
		0, // no dependencies
		0, nullptr, // no memory barriers
		0, nullptr, // no buffer memory barriers
		1, &swapChainToCompute // 1 imageMemoryBarrier
	);
	
	vkCmdDispatch(commandBuffer, SAMPLE_COUNT / KERNEL_SIZE, 1, 1);

	VkImageMemoryBarrier computeToTransfer;
	computeToTransfer.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	computeToTransfer.pNext = nullptr;
	computeToTransfer.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
	computeToTransfer.dstAccessMask = 0;
	computeToTransfer.oldLayout = VK_IMAGE_LAYOUT_GENERAL;
	computeToTransfer.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
	computeToTransfer.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	computeToTransfer.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	computeToTransfer.image = this->swapChainImages[nextSwapChainIndex];
	computeToTransfer.subresourceRange = range;

	vkCmdPipelineBarrier(
		commandBuffer,
		VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, // src stage
		VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, // dst stage
		0, // no dependencies
		0, nullptr, // no memory barriers
		0, nullptr, // no buffer memory barriers
		1, &computeToTransfer // 1 imageMemoryBarrier
	);

	if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
		throw std::runtime_error("failed to record compute command buffer!");
	}
}
auto VulkanComputeHelper::getNextImageIndex() -> u32 {
	u32 nextImageIndex;
	if (
		vkAcquireNextImageKHR(
			this->device.device(),
			this->swapChain,
			std::numeric_limits<u64>::max(),
			this->imageReadySemaphore,
			VK_NULL_HANDLE,
			&nextImageIndex
		) != VK_SUCCESS
	)
		throw std::runtime_error("failed to acquire next image!");
	return nextImageIndex;
}
auto VulkanComputeHelper::presentToWindow(u32 swapChainImageIndex, VkSemaphore toWaitOn) -> void {
	VkPresentInfoKHR presentInfo{};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	presentInfo.pNext = nullptr;
	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = &toWaitOn;
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = &this->swapChain;
	presentInfo.pImageIndices = &swapChainImageIndex;

	if (vkQueuePresentKHR(this->device.presentQueue(), &presentInfo) != VK_SUCCESS) {
		throw std::runtime_error("failed to present swapchain!");
	}
}






// read computed result
		/*{
			VkBuffer stagingBuffer;
			VkDeviceMemory stagingBufferMemory;
			const auto bufferSize = sizeof(Logistic) * SAMPLE_COUNT;

			this->createBuffer(
				bufferSize,
				VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
				stagingBuffer,
				stagingBufferMemory
			);

			void* data;
			vkMapMemory(this->device, stagingBufferMemory, 0, bufferSize, 0, &data);

			this->copyBuffer(this->shaderStorageBuffers[this->iteration % 2], stagingBuffer, bufferSize);

			vkUnmapMemory(this->device, stagingBufferMemory);
			// data has stuff now?
			Logistic* logistics = reinterpret_cast<Logistic*>(data);
			std::vector<Logistic> resultFromGPU{};
			resultFromGPU.reserve(SAMPLE_COUNT);
			for (u32 i = 0; i < SAMPLE_COUNT; i++) {
				resultFromGPU.push_back(logistics[i]);
			}
			std::cout << "one result: \n\t"
				<< "x: " << resultFromGPU.at(0).x << " r: " << resultFromGPU.at(0).r << std::endl;
			const u32 imageSize = 10000;
			u32 weirdPixelCount = 0;
			Bitmap bmp(imageSize, imageSize, 255);
			for (u32 i = 0; i < resultFromGPU.size(); i++) {
				//if (this->iteration != 1 && std::abs(0.5f - resultFromGPU.at(i).x) <= 0.001f) {
				//	std::cout << "Good point at i=" << i <<
				//		" x:" << resultFromGPU.at(i).x <<
				//		" r:" << resultFromGPU.at(i).r << " size: " << resultFromGPU.size() << std::endl;
				//}
				bmp.setPixel(
					static_cast<u32>((resultFromGPU.at(i).r - 3.5f) * (static_cast<float>(imageSize - 1) * 2.0f)),
					imageSize - static_cast<u32>(resultFromGPU.at(i).x * static_cast<float>(imageSize - 1)),
					Bitmap::RED
				);
			}
			bmp.saveBitmap("logistic_gpu_" + std::to_string(this->iteration) + "_iterations.bmp");

			vkDestroyBuffer(this->device, stagingBuffer, nullptr);
			vkFreeMemory(this->device, stagingBufferMemory, nullptr);
		}*/
		// end read computed result