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
	Window window;
	Device device;

	// createSwapChain
	std::unique_ptr<SwapChain> swapChain;

	// createComputeDescriptorSetLayout
	std::unique_ptr<DescriptorSetLayout> computeDescriptorSetLayout;
	std::unique_ptr<DescriptorSetLayout> graphicsDescriptorSetLayout;

	// createComputePipeline
	std::unique_ptr<ComputePipeline> computePipeline;
	VkPipelineLayout computePipelineLayout;

	// createComputeImage
	VkImage computeImage;
	VkImageView computeImageView;
	VkDeviceMemory computeImageMemory;
	VkSampler fragmentShaderImageSampler;

	// createGraphicsPipeline
	std::unique_ptr<GraphicsPipeline> graphicsPipeline;
	VkPipelineLayout graphicsPipelineLayout;

	// createShaderStorageBuffers
	std::vector<std::unique_ptr<Buffer>> shaderStorageBuffers;

	// createUniformBuffers
	std::unique_ptr<Buffer> uniformBuffer;

	// createDesciptorPool
	std::unique_ptr<DescriptorPool> descriptorPool;
	std::unique_ptr<DescriptorPool> graphicsDescriptorPool;

	// createComputeDescriptorSets
	std::vector<VkDescriptorSet> computeDescriptorSets;
	std::vector<VkDescriptorSet> graphicsDescriptorSets;

	// createComputeCommandBuffers
	std::vector<VkCommandBuffer> computeCommandBuffers;
	VkCommandBuffer graphicsCommandBuffer;

	VkFence computeComplete;

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
		// SwapChain
		this->createSwapChain();
		/*
			this->createImageViews();
			this->createRenderPass();
			this->createFrameBuffers();
		*/
		this->createComputeDescriptorSetLayout();
		this->createGraphicsDescriptorSetLayout();
		this->createComputePipelineLayout();
		this->createComputePipeline();
		this->createComputeImage();
		this->createGraphicsPipeline();
		
		this->createUniformBuffers();			// in ubo
		this->createShaderStorageBuffers();		// in ssbo
		
		this->createComputeDescriptorPool();
		this->createComputeDescriptorSets();
		this->createGraphicsDescriptorPool();
		this->createGraphicsDescriptorSets();
		
		this->createComputeCommandBuffers();
		this->createGraphicsCommandBuffers();
	}

	auto createSwapChain() -> void;

	auto createComputeDescriptorSetLayout() -> void;
	auto createGraphicsDescriptorSetLayout() -> void;

	auto createComputePipelineLayout() -> void;
	auto createComputePipeline() -> void;

	auto createComputeImage() -> void;

	auto createGraphicsPipeline() -> void;

	auto createShaderStorageBuffers() -> void;

	auto createUniformBuffers() -> void;

	auto createComputeDescriptorPool() -> void;

	auto createComputeDescriptorSets() -> void;

	auto createGraphicsDescriptorPool() -> void;
	auto createGraphicsDescriptorSets() -> void;

	auto createComputeCommandBuffers() -> void;
	auto createGraphicsCommandBuffers() -> void;

	auto doIteration() -> void {
		// fences are for syncing cpu and gpu. semaphores are for specifying the order of gpu tasks
		u32 imageIndex = this->getNextImageIndex(); // await graphics completion
		u32 frameIndex = imageIndex % SwapChain::MAX_FRAMES_IN_FLIGHT;
		// number of frames currently rendering and number of images in swap chain are not the same
		// imageIndex = index of image in swapchain
		// frameIndex = index of frame in flight (ie, set of buffers to use to direct gpu)
		this->recordComputeCommandBuffer(this->computeCommandBuffers[frameIndex], imageIndex);
		this->recordGraphicsCommandBuffer(this->graphicsCommandBuffer, imageIndex);

		UniformBufferObject nUbo{};
		nUbo.iteration = this->iteration;
		nUbo.pixelColor = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
		nUbo.width = static_cast<f32>(this->swapChain->getSwapChainExtent().width);
		nUbo.height = static_cast<f32>(this->swapChain->getSwapChainExtent().height);
		this->uniformBuffer->writeToBuffer(&nUbo);
		this->uniformBuffer->flush(); // make visible to device
		//std::cout << "doing iteration" << std::endl;

		/*
		{ // fake 1 second delay
			auto currentTime = std::chrono::high_resolution_clock::now();
			auto newTime = std::chrono::high_resolution_clock::now();
			while (std::chrono::duration<float, std::chrono::milliseconds::period>(newTime - currentTime).count() < 1000.0f)
				newTime = std::chrono::high_resolution_clock::now();
			std::cout << "waited 1 second probably" << std::endl;
		}
		*/

		VkSubmitInfo submitInfo = {};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

		VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
		submitInfo.waitSemaphoreCount = 0;
		submitInfo.pWaitSemaphores = nullptr;
		submitInfo.pWaitDstStageMask = waitStages;

		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &this->computeCommandBuffers[frameIndex];

		vkResetFences(this->device.device(), 1, &this->computeComplete);
		if (vkQueueSubmit(this->device.computeQueue(), 1, &submitInfo, this->computeComplete) != VK_SUCCESS)
			throw std::runtime_error("failed to submit compute command buffer!");

		vkWaitForFences(this->device.device(), 1, &this->computeComplete, VK_TRUE, UINT64_MAX);

		this->swapChain->submitCommandBuffers(&this->graphicsCommandBuffer, &imageIndex);
	}

	auto recordComputeCommandBuffer(VkCommandBuffer, u32) -> void;
	auto recordGraphicsCommandBuffer(VkCommandBuffer, u32) -> void;
	auto beginRenderPass(VkCommandBuffer, u32) -> void;
	auto endRenderPass(VkCommandBuffer) -> void;

	auto getNextImageIndex() -> u32;

public:
	VulkanComputeHelper();
	auto mainLoop() -> void {
		auto currentTime = std::chrono::high_resolution_clock::now();

		VkFenceCreateInfo fenceInfo{};
		fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
		if (vkCreateFence(this->device.device(), &fenceInfo, nullptr, &this->computeComplete) != VK_SUCCESS)
			throw std::runtime_error("failed to create fence");

		while (!this->window.shouldClose()) {
			glfwPollEvents();
			//if (this->iteration > 0) break;
			auto newTime = std::chrono::high_resolution_clock::now();
			float frameTime = std::chrono::duration<float, std::chrono::milliseconds::period>(newTime - currentTime).count();
			currentTime = newTime;
			//std::cout << "Iteration Time: " << frameTime << std::endl;
			doIteration();
			this->iteration++;
		}
		vkDeviceWaitIdle(this->device.device());
	}
	~VulkanComputeHelper();
};

VulkanComputeHelper::VulkanComputeHelper() :
	window{ 1920, 1080, "Compute-based Images" },
	device{ window }
{
	this->initVulkan();
}
VulkanComputeHelper::~VulkanComputeHelper() {
	this->computePipeline = nullptr;
	vkDestroyPipelineLayout(this->device.device(), this->computePipelineLayout, nullptr);

	this->graphicsPipeline = nullptr;
	vkDestroyPipelineLayout(this->device.device(), this->graphicsPipelineLayout, nullptr);

	vkDestroyImage(this->device.device(), this->computeImage, nullptr);
	vkDestroyImageView(this->device.device(), this->computeImageView, nullptr);
	vkDestroySampler(this->device.device(), this->fragmentShaderImageSampler, nullptr);
	vkFreeMemory(this->device.device(), this->computeImageMemory, nullptr);

	vkDestroyFence(this->device.device(), this->computeComplete, nullptr);

	this->uniformBuffer = nullptr; // deconstruct uniformBuffer
	for (int i = 0; i < this->shaderStorageBuffers.size(); i++)
		this->shaderStorageBuffers[i] = nullptr; // deconstruct ssbos
	this->computeDescriptorSetLayout = nullptr; // deconstruct descriptorSetLayout
	// deconstruct descriptor set? maybe unneeded (no errors thrown so presuming can be deconstructed whenever)
	this->descriptorPool = nullptr; // deconstruct descriptorPool
}

auto VulkanComputeHelper::createSwapChain() -> void {
	auto extent = this->window.getExtent();
	while (extent.width == 0 || extent.height == 0) {
		extent = this->window.getExtent();
		glfwWaitEvents();
	}
	vkDeviceWaitIdle(this->device.device());
	if (this->swapChain == nullptr)
		this->swapChain = std::make_unique<SwapChain>(this->device, extent);
	else {
		std::runtime_error("swap chain already create! (can use this for swap chain recreation later caused by window resize)");
	}
}

auto VulkanComputeHelper::createComputeDescriptorSetLayout() -> void {
	this->computeDescriptorSetLayout = DescriptorSetLayout::Builder(this->device)
		.addBinding(
			0,
			VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
			VK_SHADER_STAGE_COMPUTE_BIT,
			1
		).addBinding(
			1,
			VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
			VK_SHADER_STAGE_COMPUTE_BIT,
			1
		).addBinding(
			2,
			VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
			VK_SHADER_STAGE_COMPUTE_BIT,
			1
		).build();
}
auto VulkanComputeHelper::createGraphicsDescriptorSetLayout() -> void {
	this->graphicsDescriptorSetLayout = DescriptorSetLayout::Builder(this->device)
		.addBinding(
			0,
			VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			VK_SHADER_STAGE_FRAGMENT_BIT,
			1
		).build();
}

auto VulkanComputeHelper::createComputePipelineLayout() -> void {
	VkDescriptorSetLayout tempCompute = this->computeDescriptorSetLayout->getDescriptorSetLayout();
	VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = 1;
	pipelineLayoutInfo.pSetLayouts = &tempCompute;

	if (vkCreatePipelineLayout(this->device.device(), &pipelineLayoutInfo, nullptr, &this->computePipelineLayout) != VK_SUCCESS)
		throw std::runtime_error("failed to create compute pipeline layout!");
}
auto VulkanComputeHelper::createComputePipeline() -> void {
	ComputePipelineConfigInfo pipelineConfig{};
	ComputePipeline::defaultPipelineConfigInfo(pipelineConfig);
	pipelineConfig.pipelineLayout = this->computePipelineLayout;
	this->computePipeline = std::make_unique<ComputePipeline>(
		this->device,
		"shaders/compiled/logistic.comp.spv",
		pipelineConfig
	);
}

auto VulkanComputeHelper::createComputeImage() -> void {
	VkImageCreateInfo imageInfo{};
	imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageInfo.imageType = VK_IMAGE_TYPE_2D;
	imageInfo.extent.width = this->swapChain->getSwapChainExtent().width;
	imageInfo.extent.height = this->swapChain->getSwapChainExtent().height;
	imageInfo.extent.depth = 1;
	imageInfo.mipLevels = 1;
	imageInfo.arrayLayers = 1;
	imageInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
	imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
	imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	imageInfo.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
	imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
	imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	imageInfo.flags = 0;

	this->device.createImageWithInfo(
		imageInfo,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		this->computeImage,
		this->computeImageMemory
	);

	VkImageViewCreateInfo viewInfo{};
	viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	viewInfo.image = this->computeImage;
	viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	viewInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
	viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	viewInfo.subresourceRange.baseMipLevel = 0;
	viewInfo.subresourceRange.levelCount = 1;
	viewInfo.subresourceRange.baseArrayLayer = 0;
	viewInfo.subresourceRange.layerCount = 1;

	if (vkCreateImageView(this->device.device(), &viewInfo, nullptr, &this->computeImageView) != VK_SUCCESS) {
		throw std::runtime_error("failed to create texture image view!");
	}

	VkSamplerCreateInfo samplerInfo{};
	samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	samplerInfo.magFilter = VK_FILTER_LINEAR;
	samplerInfo.minFilter = VK_FILTER_LINEAR;
	samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	samplerInfo.addressModeV = samplerInfo.addressModeU;
	samplerInfo.addressModeW = samplerInfo.addressModeU;
	samplerInfo.mipLodBias = 0.0f;
	samplerInfo.maxAnisotropy = 1.0f;
	samplerInfo.minLod = 0.0f;
	samplerInfo.maxLod = 1.0f;
	samplerInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;

	if (vkCreateSampler(this->device.device(), &samplerInfo, nullptr, &this->fragmentShaderImageSampler) != VK_SUCCESS) {
		std::runtime_error("failed to create image sampler");
	}
}

auto VulkanComputeHelper::createGraphicsPipeline() -> void {
	VkDescriptorSetLayout tempGraphics = this->graphicsDescriptorSetLayout->getDescriptorSetLayout();
	VkPipelineLayoutCreateInfo layoutCreateInfo{};
	layoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	layoutCreateInfo.setLayoutCount = 1;
	layoutCreateInfo.pSetLayouts = &tempGraphics;
	if (vkCreatePipelineLayout(this->device.device(), &layoutCreateInfo, nullptr, &this->graphicsPipelineLayout) != VK_SUCCESS)
		throw std::runtime_error("Failed to create graphics pipeline layout!");
	GraphicsPipelineConfigInfo pipelineConfig{};
	GraphicsPipeline::defaultPipelineConfigInfo(pipelineConfig);
	pipelineConfig.renderPass = this->swapChain->getRenderPass();
	pipelineConfig.rasterizationInfo.cullMode = VK_CULL_MODE_FRONT_BIT;
	pipelineConfig.rasterizationInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	pipelineConfig.pipelineLayout = this->graphicsPipelineLayout;
	this->graphicsPipeline = std::make_unique<GraphicsPipeline>(
		this->device,
		"shaders/compiled/SingleTriangleFullScreen.vert.spv",
		"shaders/compiled/SingleTriangleFullScreen.frag.spv",
		pipelineConfig
	);
}

auto VulkanComputeHelper::createShaderStorageBuffers() -> void {
	// initialize logistic sample points
	std::default_random_engine rndEngine((unsigned)time(nullptr));
	std::uniform_real_distribution<float> rndDist(0.0f, 1.0f); // parameter r is 0-4, x input is 0-1

	std::vector<Logistic> points(SAMPLE_COUNT);
	for (auto& point : points) {
		point.r = (rndDist(rndEngine) * 4.0f);			// r: 3.5-4
		point.x = rndDist(rndEngine);							// x: 0-1
	}

	VkDeviceSize logisticSize = sizeof(Logistic);

	// Create Staging buffer to upload data to gpu
	Buffer stagingBuffer(
		this->device,
		logisticSize,
		points.size(),
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT, // will transfer from
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
	);

	stagingBuffer.map();
	stagingBuffer.writeToBuffer((void*)points.data());

	this->shaderStorageBuffers.push_back(
		std::make_unique<Buffer>(
			this->device,
			logisticSize,
			points.size(),
			VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, // is ssbo and will transfer into
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
		)
	);

	this->device.copyBuffer(
		this->device.graphicsQueue(),
		this->device.getGraphicsCommandPool(),
		stagingBuffer.getBuffer(),
		this->shaderStorageBuffers[this->shaderStorageBuffers.size() - 1]->getBuffer(),
		logisticSize * SAMPLE_COUNT
	);
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

auto VulkanComputeHelper::createComputeDescriptorPool() -> void {
	this->descriptorPool = DescriptorPool::Builder(this->device)
		.setMaxSets(1)
		.addPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1)
		.addPoolSize(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1)
		.addPoolSize(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1)
		.build();
}

auto VulkanComputeHelper::createComputeDescriptorSets() -> void {
	this->computeDescriptorSets.resize(1);
	auto uboBufferInfo = this->uniformBuffer->descriptorInfo(); // use same ubo for each frame cause data is const, so dont need more than one
	auto ssboBufferInfo = this->shaderStorageBuffers[0]->descriptorInfo(); // use same ssbo for each frame

	VkDescriptorImageInfo descImageInfo{};
	descImageInfo.sampler = nullptr;
	descImageInfo.imageView = this->computeImageView;
	descImageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;

	DescriptorWriter(*this->computeDescriptorSetLayout, *this->descriptorPool)
		.writeBuffer(0, &uboBufferInfo)
		.writeBuffer(1, &ssboBufferInfo)
		.writeImage(2, &descImageInfo)
		.build(this->computeDescriptorSets[0]);
}
auto VulkanComputeHelper::createGraphicsDescriptorPool() -> void {
	this->graphicsDescriptorPool = DescriptorPool::Builder(this->device)
		.setMaxSets(1)
		.addPoolSize(VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1)
		.build();
}
auto VulkanComputeHelper::createGraphicsDescriptorSets() -> void {
	this->graphicsDescriptorSets.resize(1);
	VkDescriptorImageInfo descImageInfo{};
	descImageInfo.sampler = this->fragmentShaderImageSampler;
	descImageInfo.imageView = this->computeImageView;
	descImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

	DescriptorWriter(*this->graphicsDescriptorSetLayout, *this->graphicsDescriptorPool)
		.writeImage(0, &descImageInfo)
		.build(this->graphicsDescriptorSets[0]);
}

auto VulkanComputeHelper::createComputeCommandBuffers() -> void {
	this->computeCommandBuffers.resize(SwapChain::MAX_FRAMES_IN_FLIGHT);
	VkCommandBufferAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.commandPool = this->device.getComputeCommandPool();
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandBufferCount = static_cast<u32>(SwapChain::MAX_FRAMES_IN_FLIGHT);

	if (vkAllocateCommandBuffers(this->device.device(), &allocInfo, this->computeCommandBuffers.data()) != VK_SUCCESS)
		throw std::runtime_error("Failed to allocate compute Command Buffers!");
}
auto VulkanComputeHelper::createGraphicsCommandBuffers() -> void {
	VkCommandBufferAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.commandPool = this->device.getGraphicsCommandPool();
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandBufferCount = 1;

	if (vkAllocateCommandBuffers(this->device.device(), &allocInfo, &this->graphicsCommandBuffer) != VK_SUCCESS)
		throw std::runtime_error("Failed to allocate compute Command Buffers!");
}

void VulkanComputeHelper::recordComputeCommandBuffer(VkCommandBuffer commandBuffer, u32 nextSwapChainIndex) {
	static bool firstRun = true;
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

	
	
	VkImageMemoryBarrier reset;
	reset.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	reset.pNext = nullptr;
	reset.srcAccessMask = 0;
	reset.dstAccessMask = 0;
	reset.oldLayout = firstRun ? VK_IMAGE_LAYOUT_UNDEFINED : VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	reset.newLayout = VK_IMAGE_LAYOUT_GENERAL;
	reset.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	reset.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	reset.image = this->computeImage;
	reset.subresourceRange = range;

	vkCmdPipelineBarrier(
		commandBuffer,
		VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, // src stage
		VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, // dst stage
		0, // no dependencies
		0, nullptr, // no memory barriers
		0, nullptr, // no buffer memory barriers
		1, &reset // 1 imageMemoryBarrier
	);
	


	VkClearColorValue clearValue = { 0.0f, 0.0f, 0.0f, 1.0f };
	vkCmdClearColorImage(
		commandBuffer, this->computeImage,
		VK_IMAGE_LAYOUT_GENERAL, &clearValue, 1, &range
	);
	
	
	/*
	VkImageMemoryBarrier swapChainToCompute;
	swapChainToCompute.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	swapChainToCompute.pNext = nullptr;
	swapChainToCompute.srcAccessMask = 0;
	swapChainToCompute.dstAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
	swapChainToCompute.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	swapChainToCompute.newLayout = VK_IMAGE_LAYOUT_GENERAL;
	swapChainToCompute.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	swapChainToCompute.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	swapChainToCompute.image = this->swapChain->getImage(nextSwapChainIndex);
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
	*/
	
	this->computePipeline->bind(commandBuffer);
	vkCmdBindDescriptorSets(
		commandBuffer,
		VK_PIPELINE_BIND_POINT_COMPUTE,
		this->computePipelineLayout,
		0,
		1,
		&this->computeDescriptorSets[0],
		0,
		nullptr
	);
	vkCmdDispatch(commandBuffer, SAMPLE_COUNT / KERNEL_SIZE, 1, 1);

	VkImageMemoryBarrier computeToPresent;
	computeToPresent.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	computeToPresent.pNext = nullptr;
	computeToPresent.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
	computeToPresent.dstAccessMask = 0;
	computeToPresent.oldLayout = VK_IMAGE_LAYOUT_GENERAL;
	computeToPresent.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	computeToPresent.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	computeToPresent.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	computeToPresent.image = this->computeImage;
	computeToPresent.subresourceRange = range;

	vkCmdPipelineBarrier(
		commandBuffer,
		VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, // src stage
		VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, // dst stage
		0, // no dependencies
		0, nullptr, // no memory barriers
		0, nullptr, // no buffer memory barriers
		1, &computeToPresent // 1 imageMemoryBarrier
	);
	
	if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
		throw std::runtime_error("failed to record compute command buffer!");
	}
	firstRun = false;
}
auto VulkanComputeHelper::recordGraphicsCommandBuffer(VkCommandBuffer commandBuffer, u32 currImageIndex) -> void {
	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
		throw std::runtime_error("failed to begin recording graphics command buffer!");
	}

	this->graphicsPipeline->bind(commandBuffer);
	vkCmdBindDescriptorSets(
		commandBuffer,
		VK_PIPELINE_BIND_POINT_GRAPHICS,
		this->graphicsPipelineLayout,
		0,
		1,
		&this->graphicsDescriptorSets[0],
		0,
		nullptr
	);

	this->beginRenderPass(commandBuffer, currImageIndex);
	
	vkCmdDraw(commandBuffer, 3, 1, 0, 0); // vertex shader just needs 3 vertex indexes to make space for fragment shader, so lie and say there's 3 to do even though there's none sent

	this->endRenderPass(commandBuffer);

	if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
		throw std::runtime_error("failed to record compute command buffer!");
	}
}
auto VulkanComputeHelper::beginRenderPass(VkCommandBuffer commandBuffer, u32 currImageIndex) -> void {
	VkRenderPassBeginInfo renderPassInfo{};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassInfo.renderPass = this->swapChain->getRenderPass();
	renderPassInfo.framebuffer = this->swapChain->getFrameBuffer(currImageIndex);
	renderPassInfo.renderArea.offset = { 0, 0 };
	renderPassInfo.renderArea.extent = this->swapChain->getSwapChainExtent();
	
	std::array<VkClearValue, 2> clearValues{};
	clearValues[0].color = { 0.01f, 0.01f, 0.01f, 1.0f };
	clearValues[1].depthStencil = { 1.0f, 0 };
	renderPassInfo.clearValueCount = 2;
	renderPassInfo.pClearValues = clearValues.data();

	vkCmdBeginRenderPass(
		commandBuffer,
		&renderPassInfo,
		VK_SUBPASS_CONTENTS_INLINE
	);

	VkViewport viewport{};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = static_cast<float>(this->swapChain->getSwapChainExtent().width);
	viewport.height = static_cast<float>(this->swapChain->getSwapChainExtent().height);
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;
	VkRect2D scissor{ {0, 0}, this->swapChain->getSwapChainExtent() };
	vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
	vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
}
auto VulkanComputeHelper::endRenderPass(VkCommandBuffer commandBuffer) -> void {
	vkCmdEndRenderPass(commandBuffer);
}
auto VulkanComputeHelper::getNextImageIndex() -> u32 {
	u32 nextImageIndex;
	if (
		this->swapChain->acquireNextImage(&nextImageIndex) != VK_SUCCESS
	)
		throw std::runtime_error("failed to acquire next image!");
	return nextImageIndex;
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