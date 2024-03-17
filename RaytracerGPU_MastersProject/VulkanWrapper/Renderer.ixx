module;

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <cassert>

export module VulkanWrap:Renderer;

import PrimitiveTypes;

import :Device;
import :Window;
import :SwapChain;

import <vector>;
import <memory>;
import <array>;
import <stdexcept>;

/*
	In charge of managing swapchain and commandbuffers
*/
export class Renderer {
	Window& window;
	Device& device;
	std::unique_ptr<SwapChain> swapChain;
	std::vector<VkCommandBuffer> commandBuffers;

	u32 currentImageIndex{ 0 };
	i32 currentFrameIndex{ 0 };
	bool isFrameStarted{ false };

	auto createCommandBuffers() -> void;
	auto freeCommandBuffers() -> void;
	auto recreateSwapChain() -> void;
public:
	Renderer(Window& window, Device& device);
	~Renderer();

	Renderer(const Renderer&) = delete;
	Renderer& operator=(const Renderer&) = delete;

	int getFrameIndex() const {
		assert(this->isFrameStarted && "Cannot get frame index when frame not in progress");
		return this->currentFrameIndex;
	}

	auto beginFrame() -> VkCommandBuffer;
	auto endFrame() -> void;
	auto beginSwapChainRenderPass(VkCommandBuffer commandBuffer) -> void;
	auto endSwapChainRenderPass(VkCommandBuffer commandBuffer) -> void;

	auto getSwapChainRenderPass() const -> VkRenderPass {
		return this->swapChain->getRenderPass();
	}
	auto getAspectRatio() const -> f32 {
		return this->swapChain->extentAspectRatio();
	}
	auto isFrameInProgress() const -> bool {
		return this->isFrameStarted;
	}
	auto getCurrentCommandBuffer() const -> VkCommandBuffer {
		assert(this->isFrameStarted && "Cannot get command buffer when frame not in progress");
		return this->commandBuffers[this->currentFrameIndex];
	}
};
