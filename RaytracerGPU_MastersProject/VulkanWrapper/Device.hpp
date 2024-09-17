#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "Window.hpp"

#include <string>
#include <vector>

struct SwapChainSupportDetails {
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
};

struct QueueFamilyIndices {
    uint32_t computeFamily;
    uint32_t graphicsFamily;
    uint32_t presentFamily;
    bool computeFamilyHasValue = false;
    bool graphicsFamilyHasValue = false;
    bool presentFamilyHasValue = false;
    bool isComplete() { return computeFamilyHasValue && graphicsFamilyHasValue && presentFamilyHasValue; }
};

class Device {
    VkInstance instance;
    VkDebugUtilsMessengerEXT debugMessenger;
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    Window& window;
    VkCommandPool graphicsCommandPool;
    VkCommandPool computeCommandPool;
    bool graphicsAndComputeSameQueueFamily;

    VkDevice device_;
    VkSurfaceKHR surface_;
    QueueFamilyIndices queueFamilyCache;
    VkQueue computeQueue_;
    VkQueue graphicsQueue_;
    VkQueue presentQueue_;

    const std::vector<const char*> validationLayers = { "VK_LAYER_KHRONOS_validation" };
    const std::vector<const char*> deviceExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

    void createInstance();
    void setupDebugMessenger();
    void createSurface();
    void pickPhysicalDevice();
    void createLogicalDevice();
    void createCommandPools();

    // helper functions
    bool isDeviceSuitable(VkPhysicalDevice device);
    std::vector<const char*> getRequiredExtensions();
    bool checkValidationLayerSupport();
    QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device);
    void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);
    void hasGflwRequiredInstanceExtensions();
    bool checkDeviceExtensionSupport(VkPhysicalDevice device);
    SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device);

public:
#ifdef NDEBUG
    const bool enableValidationLayers = false;
#else
    const bool enableValidationLayers = true;
#endif

    Device(Window& window);
    ~Device();

    // Not copyable or movable
    Device(const Device&) = delete;
    Device& operator=(const Device&) = delete;
    Device(Device&&) = delete;
    Device& operator=(Device&&) = delete;

    VkCommandPool getGraphicsCommandPool() { return this->graphicsCommandPool; }
    VkCommandPool getComputeCommandPool() { return this->graphicsAndComputeSameQueueFamily ? this->graphicsCommandPool : this->computeCommandPool; }
    VkDevice device() { return this->device_; }
    VkSurfaceKHR surface() { return this->surface_; }
    VkQueue computeQueue() { return this->computeQueue_; }
    VkQueue graphicsQueue() { return this->graphicsQueue_; }
    VkQueue presentQueue() { return this->presentQueue_; }

    SwapChainSupportDetails getSwapChainSupport() { return this->querySwapChainSupport(physicalDevice); }
    uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);
    QueueFamilyIndices findPhysicalQueueFamilies() { return this->findQueueFamilies(physicalDevice); }
    VkFormat findSupportedFormat(
        const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features);

    // Buffer Helper Functions
    void createBuffer(
        VkDeviceSize size,
        VkBufferUsageFlags usage,
        VkMemoryPropertyFlags properties,
        VkBuffer& buffer,
        VkDeviceMemory& bufferMemory);
    VkCommandBuffer beginSingleTimeCommands(VkCommandPool pool);
    void endSingleTimeCommands(VkQueue queue, VkCommandPool pool, VkCommandBuffer commandBuffer);
    void copyBuffer(VkQueue queue, VkCommandPool pool, VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);
    void copyBufferToImage(
        VkQueue queue, VkCommandPool pool, VkBuffer buffer, VkImage image, uint32_t width, uint32_t height, uint32_t layerCount);

    void createImageWithInfo(
        const VkImageCreateInfo& imageInfo,
        VkMemoryPropertyFlags properties,
        VkImage& image,
        VkDeviceMemory& imageMemory);

    VkPhysicalDeviceProperties properties;
};
