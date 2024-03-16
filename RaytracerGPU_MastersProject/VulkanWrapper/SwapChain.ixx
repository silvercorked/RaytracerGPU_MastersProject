module;

// vulkan headers
#include <vulkan/vulkan.h>

export module VulkanWrap:SwapChain;

import :Device;

// std lib headers
import <vector>;
import <memory>;

export class SwapChain {
    VkFormat swapChainImageFormat;
    VkFormat swapChainDepthFormat;
    VkExtent2D swapChainExtent;

    std::vector<VkFramebuffer> swapChainFramebuffers;
    VkRenderPass renderPass;

    std::vector<VkImage> depthImages;
    std::vector<VkDeviceMemory> depthImageMemorys;
    std::vector<VkImageView> depthImageViews;
    std::vector<VkImage> swapChainImages;
    std::vector<VkImageView> swapChainImageViews;

    Device& device;
    VkExtent2D windowExtent;

    VkSwapchainKHR swapChain;
    std::shared_ptr<SwapChain> oldSwapChain;

    std::vector<VkSemaphore> imageAvailableSemaphores;
    std::vector<VkSemaphore> renderFinishedSemaphores;
    std::vector<VkFence> inFlightFences;
    std::vector<VkFence> imagesInFlight;
    size_t currentFrame = 0;

public:
    static constexpr int MAX_FRAMES_IN_FLIGHT = 2;

    SwapChain(Device& deviceRef, VkExtent2D windowExtent);
    SwapChain(Device& deviceRef, VkExtent2D windowExtent, std::shared_ptr<SwapChain> previous);
    ~SwapChain();

    SwapChain(const SwapChain&) = delete;
    SwapChain& operator=(const SwapChain&) = delete;

    VkFramebuffer getFrameBuffer(int index) { return this->swapChainFramebuffers[index]; }
    VkRenderPass getRenderPass() { return this->renderPass; }
    VkImage getImage(int index) { return this->swapChainImages[index]; }
    VkImageView getImageView(int index) { return this->swapChainImageViews[index]; }
    size_t imageCount() { return this->swapChainImages.size(); }
    VkFormat getSwapChainImageFormat() { return this->swapChainImageFormat; }
    VkExtent2D getSwapChainExtent() { return this->swapChainExtent; }
    uint32_t width() { return this->swapChainExtent.width; }
    uint32_t height() { return this->swapChainExtent.height; }

    VkFormat findDepthFormat();
    VkResult acquireNextImage(uint32_t* imageIndex);
    VkResult submitCommandBuffers(const VkCommandBuffer* buffers, uint32_t* imageIndex);

    float extentAspectRatio() {
        return static_cast<float>(this->swapChainExtent.width) / static_cast<float>(this->swapChainExtent.height);
    }
    bool compareSwapFormats(const SwapChain& swapChain) const { // if true, this and given are compatibly SwapChains
        return swapChain.swapChainDepthFormat == this->swapChainDepthFormat && swapChain.swapChainImageFormat == this->swapChainImageFormat;
    }

private:
    auto init() -> void;
    auto createSwapChain() -> void;
    auto createImageViews() -> void;
    auto createDepthResources() -> void;
    auto createRenderPass() -> void;
    auto createFramebuffers() -> void;
    auto createSyncObjects() -> void;

    // Helper functions
    auto chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats) -> VkSurfaceFormatKHR;
    auto chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes) -> VkPresentModeKHR;
    auto chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities) -> VkExtent2D;
};
