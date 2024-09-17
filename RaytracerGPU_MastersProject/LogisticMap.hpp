#pragma once

#include "utils/Bitmap.hpp"

//#include <vulkan/vulkan.h>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS					// functions expect radians, not degrees
#define GLM_FORCE_DEPTH_ZERO_TO_ONE			// Depth buffer values will range from 0 to 1, not -1 to 1
#include <glm/glm.hpp>

#include "utils/PrimitiveTypes.hpp"
#include "VulkanWrapper/Window.hpp"
#include "VulkanWrapper/Device.hpp"
#include "VulkanWrapper/SwapChain.hpp"
#include "VulkanWrapper/ComputePipeline.hpp"
#include "VulkanWrapper/GraphicsPipeline.hpp"
#include "VulkanWrapper/Buffer.hpp"
#include "VulkanWrapper/Descriptors.hpp"

#include <chrono>

namespace LogisticMapRenderer {
	constexpr const u32 SAMPLE_COUNT = 1024 * 512;
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

	class LogisticMap {
		Window window;
		Device device;
		//Renderer renderer{ window, device };

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

			LogisticMapRenderer::UniformBufferObject nUbo{};
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
		LogisticMap();
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
		~LogisticMap();
	};
};
