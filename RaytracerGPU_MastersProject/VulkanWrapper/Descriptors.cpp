
#include "Descriptors.hpp"

#include <cassert>
#include <stdexcept>

auto DescriptorSetLayout::Builder::addBinding(
	uint32_t binding,
	VkDescriptorType descriptorType,
	VkShaderStageFlags stageFlags,
	uint32_t count
) -> DescriptorSetLayout::Builder& {
	assert(this->bindings.count(binding) == 0 && "Binding already in use");
	VkDescriptorSetLayoutBinding layoutBinding{};
	layoutBinding.binding = binding;
	layoutBinding.descriptorType = descriptorType;
	layoutBinding.descriptorCount = count;
	layoutBinding.stageFlags = stageFlags;
	this->bindings[binding] = layoutBinding;
	return *this;
}
auto DescriptorSetLayout::Builder::build() const -> std::unique_ptr<DescriptorSetLayout> {
	return std::make_unique<DescriptorSetLayout>(this->device, this->bindings);
}

DescriptorSetLayout::DescriptorSetLayout(
	Device& device, std::unordered_map<uint32_t, VkDescriptorSetLayoutBinding> bindings
) :
	device{ device }, bindings{ bindings }
{
	std::vector<VkDescriptorSetLayoutBinding> setLayoutBindings{};
	for (auto kv : bindings) {
		setLayoutBindings.push_back(kv.second);
	}

	VkDescriptorSetLayoutCreateInfo descriptorSetLayoutInfo{}; // seems like a common pattern, createinfo struct, call create function, call destroy function
	descriptorSetLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	descriptorSetLayoutInfo.bindingCount = static_cast<uint32_t>(setLayoutBindings.size());
	descriptorSetLayoutInfo.pBindings = setLayoutBindings.data();

	if (
		vkCreateDescriptorSetLayout(
			this->device.device(),
			&descriptorSetLayoutInfo,
			nullptr,
			&this->descriptorSetLayout
		) != VK_SUCCESS
	) {
		throw std::runtime_error("failed to create descriptor set layout!");
	}
}
DescriptorSetLayout::~DescriptorSetLayout() {
	vkDestroyDescriptorSetLayout(this->device.device(), this->descriptorSetLayout, nullptr);
}

auto DescriptorPool::Builder::addPoolSize(
	VkDescriptorType descriptorType, uint32_t count
) -> DescriptorPool::Builder& {
	this->poolSizes.push_back({ descriptorType, count });
	return *this;
}
auto DescriptorPool::Builder::setPoolFlags(
	VkDescriptorPoolCreateFlags flags
) -> DescriptorPool::Builder& {
	this->poolFlags = flags;
	return *this;
}
auto DescriptorPool::Builder::setMaxSets(uint32_t count) -> DescriptorPool::Builder& {
	this->maxSets = count;
	return *this;
}
auto DescriptorPool::Builder::build() const -> std::unique_ptr<DescriptorPool> {
	return std::make_unique<DescriptorPool>(this->device, this->maxSets, this->poolFlags, this->poolSizes);
}

DescriptorPool::DescriptorPool(
	Device& device,
	uint32_t maxSets,
	VkDescriptorPoolCreateFlags poolFlags,
	const std::vector<VkDescriptorPoolSize>& poolSizes
) :
	device{ device }
{
	VkDescriptorPoolCreateInfo descriptorPoolInfo{};
	descriptorPoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	descriptorPoolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
	descriptorPoolInfo.pPoolSizes = poolSizes.data();
	descriptorPoolInfo.maxSets = maxSets;
	descriptorPoolInfo.flags = poolFlags;

	if (vkCreateDescriptorPool(this->device.device(), &descriptorPoolInfo, nullptr, &this->descriptorPool) != VK_SUCCESS) {
		throw std::runtime_error("failed to create descriptor pool!");
	}
}
DescriptorPool::~DescriptorPool() {
	vkDestroyDescriptorPool(this->device.device(), this->descriptorPool, nullptr);
}
auto DescriptorPool::allocateDescriptor(
	const VkDescriptorSetLayout descriptorSetLayout,
	VkDescriptorSet& descriptor
) const -> bool {
	VkDescriptorSetAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = this->descriptorPool;
	allocInfo.pSetLayouts = &descriptorSetLayout;
	allocInfo.descriptorSetCount = 1;

	// may want a descriptor pool manager class that handles creating a new pool when the old one fills up
	if (vkAllocateDescriptorSets(this->device.device(), &allocInfo, &descriptor) != VK_SUCCESS) {
		return false;
	}
	return true;
}
auto DescriptorPool::freeDescriptors(std::vector<VkDescriptorSet>& descriptors) const -> void {
	vkFreeDescriptorSets(
		this->device.device(),
		this->descriptorPool,
		static_cast<uint32_t>(descriptors.size()),
		descriptors.data()
	);
}
auto DescriptorPool::resetPool() -> void {
	vkResetDescriptorPool(this->device.device(), this->descriptorPool, 0);
}

DescriptorWriter::DescriptorWriter(
	DescriptorSetLayout& setLayout,
	DescriptorPool& pool
) : setLayout{ setLayout }, pool{ pool } {}
auto DescriptorWriter::writeBuffer(
	uint32_t binding,
	VkDescriptorBufferInfo* bufferInfo
) -> DescriptorWriter& {
	assert(this->setLayout.bindings.count(binding) == 1 && "Layout does not contain specified binding");
	auto& bindingDescription = this->setLayout.bindings[binding];
	assert(bindingDescription.descriptorCount == 1 && "Binding single descriptor info, but binding expects multiple");

	VkWriteDescriptorSet write{};
	write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	write.descriptorType = bindingDescription.descriptorType;
	write.dstBinding = binding;
	write.pBufferInfo = bufferInfo;
	write.descriptorCount = 1;
	this->writes.push_back(write);
	return *this;
}
auto DescriptorWriter::writeImage(
	uint32_t binding,
	VkDescriptorImageInfo* imageInfo
) -> DescriptorWriter& {
	assert(this->setLayout.bindings.count(binding) == 1 && "Layout does not contain specified binding");
	auto& bindingDescription = this->setLayout.bindings[binding];
	assert(bindingDescription.descriptorCount == 1 && "Binding single descriptor info, but binding expects multiple");

	VkWriteDescriptorSet write{};
	write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	write.descriptorType = bindingDescription.descriptorType;
	write.dstBinding = binding;
	write.pImageInfo = imageInfo;
	write.descriptorCount = 1;
	this->writes.push_back(write);
	return *this;
}
auto DescriptorWriter::build(VkDescriptorSet& set) -> bool {
	bool success = this->pool.allocateDescriptor(this->setLayout.getDescriptorSetLayout(), set);
	if (!success) return false;
	this->overwrite(set);
	return true;
}
auto DescriptorWriter::overwrite(VkDescriptorSet& set) -> void {
	for (auto& write : this->writes)
		write.dstSet = set;
	vkUpdateDescriptorSets(this->pool.device.device(), this->writes.size(), this->writes.data(), 0, nullptr);
}
