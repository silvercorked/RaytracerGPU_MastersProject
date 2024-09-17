#pragma once

#include <vulkan/vulkan.h>

#include "Device.hpp"

class Buffer {
	Device& _device;
	void* _mapped = nullptr;
	VkBuffer _buffer = VK_NULL_HANDLE;
	VkDeviceMemory _memory = VK_NULL_HANDLE;

	VkDeviceSize _bufferSize;
	uint32_t _instanceCount;
	VkDeviceSize _instanceSize;
	VkDeviceSize _alignmentSize;
	VkBufferUsageFlags _usageFlags;
	VkMemoryPropertyFlags _memoryPropertyFlags;

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

	auto getBuffer() const -> VkBuffer { return this->_buffer; }
	auto getMappedMemory() const -> void* { return this->_mapped; }
	auto getInstanceCount() const -> uint32_t { return this->_instanceCount; }
	auto getInstanceSize() const -> VkDeviceSize { return this->_instanceSize; }
	auto getAlignmentSize() const -> VkDeviceSize { return this->_alignmentSize; }
	auto getUsageFlags() const -> VkBufferUsageFlags { return this->_usageFlags; }
	auto getMemoryPropertyFlags() const -> VkMemoryPropertyFlags { return this->_memoryPropertyFlags; }
	auto getBufferSize() const -> VkDeviceSize { return this->_bufferSize; }
};
