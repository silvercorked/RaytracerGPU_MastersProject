module;

#include <vulkan/vulkan.h>

export module VulkanWrap:Buffer;

import :Device;

import <cstring>;

export class Buffer {
	Device& device;
	void* mapped = nullptr;
	VkBuffer buffer = VK_NULL_HANDLE;
	VkDeviceMemory memory = VK_NULL_HANDLE;

	VkDeviceSize bufferSize;
	uint32_t instanceCount;
	VkDeviceSize instanceSize;
	VkDeviceSize alignmentSize;
	VkBufferUsageFlags usageFlags;
	VkMemoryPropertyFlags memoryPropertyFlags;

	static auto getAlignment(VkDeviceSize instanceSize, VkDeviceSize minOffsetAlignment) -> VkDeviceSize;

public:
	Buffer(
		Device& device,
		VkDeviceSize instanceSize,
		uint32_t instanceCount,
		VkBufferUsageFlags usageFlags,
		VkMemoryPropertyFlags memoryPropertyFlags,
		VkDeviceSize minOffsetAlignment = 1
	);

	Buffer(const Buffer&) = delete;
	Buffer& operator=(const Buffer&) = delete;

	~Buffer();

	auto map(VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0) -> VkResult;
	auto unmap() -> void;

	auto writeToBuffer(void* data, VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0) -> void;
	auto flush(VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0) -> VkResult;
	auto descriptorInfo(VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0) -> VkDescriptorBufferInfo;
	auto invalidate(VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0) -> VkResult;

	auto writeToIndex(void* data, int index) -> void;
	auto flushIndex(int index) -> VkResult;
	auto descriptorInfoForIndex(int index) -> VkDescriptorBufferInfo;
	auto invalidateIndex(int index) -> VkResult;

	auto getBuffer() const -> VkBuffer { return this->buffer; }
	auto getMappedMemory() const -> void* { return this->mapped; }
	auto getInstanceCount() const -> uint32_t { return this->instanceCount; }
	auto getInstanceSize() const -> VkDeviceSize { return this->instanceSize; }
	auto getAlignmentSize() const -> VkDeviceSize { return this->alignmentSize; }
	auto getUsageFlags() const -> VkBufferUsageFlags { return this->usageFlags; }
	auto getMemoryPropertyFlags() const -> VkMemoryPropertyFlags { return this->memoryPropertyFlags; }
	auto getBufferSize() const -> VkDeviceSize { return this->bufferSize; }
};
