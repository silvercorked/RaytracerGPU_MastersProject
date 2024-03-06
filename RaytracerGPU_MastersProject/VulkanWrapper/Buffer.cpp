
#include <vulkan/vulkan.h>

#include <cassert>

module VulkanWrap:Buffer;

// smallest size in bytes that satisfies alignment
/*
	example: instanceSize = 19 bytes, minAlignment = 16bytes,
	32 bytes per instance must be used and is returned by this function
*/
auto Buffer::getAlignment(VkDeviceSize instanceSize, VkDeviceSize minOffsetAlignment) -> VkDeviceSize {
	if (minOffsetAlignment > 0)
		return (instanceSize + minOffsetAlignment - 1) & ~(minOffsetAlignment - 1);
	return instanceSize;
}

Buffer::Buffer(
	Device& device,
	VkDeviceSize instanceSize,
	uint32_t instanceCount,
	VkBufferUsageFlags usageFlags,
	VkMemoryPropertyFlags memoryPropertyFlags,
	VkDeviceSize minOffsetAlignment
) :
	device{ device },
	instanceSize{ instanceSize },
	instanceCount{ instanceCount },
	usageFlags{ usageFlags },
	memoryPropertyFlags{ memoryPropertyFlags }
{
	this->alignmentSize = getAlignment(instanceSize, minOffsetAlignment);
	this->bufferSize = this->alignmentSize * instanceCount;
	device.createBuffer(this->bufferSize, this->usageFlags, this->memoryPropertyFlags, this->buffer, this->memory);
}

Buffer::~Buffer() {
	this->unmap();
	vkDestroyBuffer(this->device.device(), this->buffer, nullptr);
	vkFreeMemory(this->device.device(), this->memory, nullptr);
}

/**
	* Map a memory range of this buffer. If successful, mapped points to the specified buffer range.
	*
	* @param size (Optional) Size of the memory range to map. Pass VK_WHOLE_SIZE to map the complete
	* buffer range.
	* @param offset (Optional) Byte offset from beginning
	*
	* @return VkResult of the buffer mapping call
	*/
auto Buffer::map(VkDeviceSize size, VkDeviceSize offset) -> VkResult {
	assert(this->buffer && this->memory && "Called map on buffer before create");
	return vkMapMemory(this->device.device(), this->memory, offset, size, 0, &this->mapped);
}
/**
	* Unmap a mapped memory range
	*
	* @note Does not return a result as vkUnmapMemory can't fail
	*/
auto Buffer::unmap() -> void {
	if (this->mapped) {
		vkUnmapMemory(this->device.device(), this->memory);
		this->mapped = nullptr;
	}
}

/**
	* Copies the specified data to the mapped buffer. Default value writes whole buffer range
	*
	* @param data Pointer to the data to copy
	* @param size (Optional) Size of the data to copy. Pass VK_WHOLE_SIZE to flush the complete buffer
	* range.
	* @param offset (Optional) Byte offset from beginning of mapped region
	*
	*/
auto Buffer::writeToBuffer(void* data, VkDeviceSize size, VkDeviceSize offset) -> void {
	assert(this->mapped && "Cannot copy to unmapped buffer");

	if (size == VK_WHOLE_SIZE)
		memcpy(this->mapped, data, this->bufferSize);
	else {
		char* memOffset = (char*)this->mapped;
		memOffset += offset;
		memcpy(memOffset, data, size);
	}
}
/**
	* Flush a memory range of the buffer to make it visible to the device
	*
	* @note Only required for non-coherent memory
	*
	* @param size (Optional) Size of the memory range to flush. Pass VK_WHOLE_SIZE to flush the
	* complete buffer range.
	* @param offset (Optional) Byte offset from beginning
	*
	* @return VkResult of the flush call
	*/
auto Buffer::flush(VkDeviceSize size, VkDeviceSize offset) -> VkResult {
	VkMappedMemoryRange mappedRange = {};
	mappedRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
	mappedRange.memory = this->memory;
	mappedRange.offset = offset;
	mappedRange.size = size;
	return vkFlushMappedMemoryRanges(this->device.device(), 1, &mappedRange);
}
/**
	* Invalidate a memory range of the buffer to make it visible to the host
	*
	* @note Only required for non-coherent memory
	*
	* @param size (Optional) Size of the memory range to invalidate. Pass VK_WHOLE_SIZE to invalidate
	* the complete buffer range.
	* @param offset (Optional) Byte offset from beginning
	*
	* @return VkResult of the invalidate call
	*/
auto Buffer::descriptorInfo(VkDeviceSize size, VkDeviceSize offset) -> VkDescriptorBufferInfo {
	return VkDescriptorBufferInfo{ this->buffer, offset, size };
}
/**
	* Create a buffer info descriptor
	*
	* @param size (Optional) Size of the memory range of the descriptor
	* @param offset (Optional) Byte offset from beginning
	*
	* @return VkDescriptorBufferInfo of specified offset and range
	*/
auto Buffer::invalidate(VkDeviceSize size, VkDeviceSize offset) -> VkResult {
	VkMappedMemoryRange mappedRange = {};
	mappedRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
	mappedRange.memory = this->memory;
	mappedRange.offset = offset;
	mappedRange.size = size;
	return vkInvalidateMappedMemoryRanges(this->device.device(), 1, &mappedRange);
}
/**
	* Copies "instanceSize" bytes of data to the mapped buffer at an offset of index * alignmentSize
	*
	* @param data Pointer to the data to copy
	* @param index Used in offset calculation
	*
	*/
auto Buffer::writeToIndex(void* data, int index) -> void {
	this->writeToBuffer(data, this->instanceSize, index * this->alignmentSize);
}
/**
	*  Flush the memory range at index * alignmentSize of the buffer to make it visible to the device
	*
	* @param index Used in offset calculation
	*
	*/
auto Buffer::flushIndex(int index) -> VkResult {
	return this->flush(this->alignmentSize, index * alignmentSize);
}
/**
	* Create a buffer info descriptor
	*
	* @param index Specifies the region given by index * alignmentSize
	*
	* @return VkDescriptorBufferInfo for instance at index
	*/
auto Buffer::descriptorInfoForIndex(int index) -> VkDescriptorBufferInfo {
	return this->descriptorInfo(this->alignmentSize, index * this->alignmentSize);
}
/**
	* Invalidate a memory range of the buffer to make it visible to the host
	*
	* @note Only required for non-coherent memory
	*
	* @param index Specifies the region to invalidate: index * alignmentSize
	*
	* @return VkResult of the invalidate call
	*/
auto Buffer::invalidateIndex(int index) -> VkResult {
	return this->invalidate(this->alignmentSize, index * this->alignmentSize);
}
