
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <cassert>

module VulkanWrap:Renderer;

Renderer::Renderer(Window& w, Device& d) : window{ w }, device{ d } {
	this->recreateSwapChain(); // calls create pipeline
	this->createCommandBuffers();
}
Renderer::~Renderer() {
	this->freeCommandBuffers();
}

auto Renderer::recreateSwapChain() -> void {
	auto extent = this->window.getExtent();
	while (extent.width == 0 || extent.height == 0) {	// if one dimension is sizeless
		extent = this->window.getExtent();
		glfwWaitEvents();								// pause and wait
	}
	vkDeviceWaitIdle(this->device.device());	// wait swapchain to be idle
	if (this->swapChain == nullptr)
		this->swapChain = std::make_unique<SwapChain>(this->device, extent);
	else {
		std::shared_ptr<SwapChain> oldSwapChain = std::move(this->swapChain);
		this->swapChain = std::make_unique <SwapChain>(this->device, extent, oldSwapChain);
		if (!oldSwapChain->compareSwapFormats(*this->swapChain.get())) {
			throw std::runtime_error("Swap chain image(or depth) format has changed!");
		}
	}
	// be back soon
}
auto Renderer::createCommandBuffers() -> void {
	/*
		Command buffers hold commands for the api
		Pros:
			- often, not much changes in terms of what needs to be done each frame, so this can be created
				once and then reused every frame.

		Lifecycle State Diagram:
			(State) on (Event) -> (next State)
			Initial on Begin -> Recording
			Recording on Reset -> Initial
			Recording on Invalidate -> Invalid
			Recording on End -> Executable
			Executable on Reset -> Initial
			Executable on Invalidate -> Invalid
			Executable on Submission -> Pending
			Pending on OneTimeSubmit -> Invalid
			Pending on Completion -> Executable
			Invalid on Reset -> Initial

		*note: undefined behavior to call Submit on a Command Buffer in the Pending State
	*/
	this->commandBuffers.resize(SwapChain::MAX_FRAMES_IN_FLIGHT); // one to one to avoid re-Recording command buffers as they target a frame buffer
	VkCommandBufferAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;					// primary: can be submitted to device for submission, can call secondary command buffers
	// secondary: cannot be submitted to device for submission, but can be called by primary command buffers
	allocInfo.commandPool = this->device.getCommandPool(); // avoid allocation and deallocating command buffer memory using command pools
	allocInfo.commandBufferCount = static_cast<uint32_t>(this->commandBuffers.size());

	if (
		vkAllocateCommandBuffers(
			this->device.device(),
			&allocInfo,
			this->commandBuffers.data()
		) != VK_SUCCESS
		) {
		throw std::runtime_error("Failed to allocate command buffers");
	}
}
auto Renderer::freeCommandBuffers() -> void {
	vkFreeCommandBuffers(
		this->device.device(),
		this->device.getCommandPool(),
		static_cast<uint32_t>(this->commandBuffers.size()),
		this->commandBuffers.data()
	);
	this->commandBuffers.clear();
}
auto Renderer::beginFrame() -> VkCommandBuffer {
	assert(!this->isFrameStarted && "Can't call beginFrame while already in progress");
	auto result = this->swapChain->acquireNextImage(&this->currentImageIndex);

	if (result == VK_ERROR_OUT_OF_DATE_KHR) {	// is thrown after window resize and in some other cases
		this->recreateSwapChain();				// requires swapchain recreation
		return nullptr;							// means frame has not successfully started
	}
	if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
		throw std::runtime_error("failed to acquire swap chain image");
	}

	this->isFrameStarted = true;
	auto commandBuffer = this->getCurrentCommandBuffer();
	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

	if (
		vkBeginCommandBuffer( // call Begin event to transition from Initial to Recording
			commandBuffer,
			&beginInfo
		) != VK_SUCCESS
		) {
		throw std::runtime_error("failed to transition command buffer to recording state");
	}
	return commandBuffer;
}
auto Renderer::endFrame() -> void {
	assert(this->isFrameStarted && "Can't call endFrame while frame is not in progress");
	auto commandBuffer = getCurrentCommandBuffer();
	if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
		throw std::runtime_error("failed to transition command buffer to executable state");
	}

	auto result = this->swapChain->submitCommandBuffers(&commandBuffer, &this->currentImageIndex);
	if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || this->window.wasWindowResized()) {
		this->window.resetWindowResizeFlag();
		this->recreateSwapChain();
	}
	else if (result != VK_SUCCESS) {
		throw std::runtime_error("failed to present swap chain image");
	}

	this->isFrameStarted = false;
	this->currentFrameIndex = (this->currentFrameIndex + 1) % SwapChain::MAX_FRAMES_IN_FLIGHT;
}
auto Renderer::beginSwapChainRenderPass(VkCommandBuffer commandBuffer) -> void {
	assert(this->isFrameStarted && "Can't call beginSwapChainRenderPass while frame is not in progress");
	assert(commandBuffer == this->getCurrentCommandBuffer() && "Can't begin render pass on commandbuffer from a different frame");

	VkRenderPassBeginInfo renderPassInfo{};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassInfo.renderPass = this->swapChain->getRenderPass();
	renderPassInfo.framebuffer = this->swapChain->getFrameBuffer(this->currentImageIndex); // associate with frame buffer

	/*
		framebuffers have 2 parts so far, color (index 0) and depth (index 1). This is according to structure given in renderPass
	*/

	renderPassInfo.renderArea.offset = { 0, 0 };
	renderPassInfo.renderArea.extent = this->swapChain->getSwapChainExtent();

	std::array<VkClearValue, 2> clearValues{};
	clearValues[0].color = { 0.01f, 0.01f, 0.01f, 1.0f };
	clearValues[1].depthStencil = { 1.0f, 0 };
	renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
	renderPassInfo.pClearValues = clearValues.data();

	vkCmdBeginRenderPass(
		commandBuffer,
		&renderPassInfo,				// record render pass to command buffer
		VK_SUBPASS_CONTENTS_INLINE		// only a primary command buffer will be used, no secondaries
	);									// VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS if ^ isn't the case
	// can't have a command buffer that has inline commands and secondary command buffers

	VkViewport viewport{};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = static_cast<float>(this->swapChain->getSwapChainExtent().width);
	viewport.height = static_cast<float>(this->swapChain->getSwapChainExtent().height);
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;
	VkRect2D scissor{ {0, 0}, this->swapChain->getSwapChainExtent() };
	vkCmdSetViewport(commandBuffer, 0, 1, &viewport);		// set viewport just before executing each frame
	vkCmdSetScissor(commandBuffer, 0, 1, &scissor);		// set scissor just before executing each frame
	// 0 for viewport index, 1 for viewport count
}
auto Renderer::endSwapChainRenderPass(VkCommandBuffer commandBuffer) -> void {
	assert(this->isFrameStarted && "Can't call endSwapChainRenderPass while frame is not in progress");
	assert(commandBuffer == this->getCurrentCommandBuffer() && "Can't end render pass on commandbuffer from a different frame");
	vkCmdEndRenderPass(commandBuffer); // call End event to transition from Recording to Executable
}
