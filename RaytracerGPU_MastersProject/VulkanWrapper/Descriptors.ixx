module;

#include <vulkan/vulkan.h>

export module VulkanWrap:Descriptors;

import :Device;

// std lib headers
import <memory>;
import <unordered_map>;
import <vector>;
import <stdexcept>;

export class DescriptorSetLayout {
	Device& device;
	VkDescriptorSetLayout descriptorSetLayout;
	std::unordered_map<uint32_t, VkDescriptorSetLayoutBinding> bindings;

	friend class DescriptorWriter;

public:
	class Builder { // convenience for creating
		Device& device;
		std::unordered_map<uint32_t, VkDescriptorSetLayoutBinding> bindings{};

	public:
		Builder(Device& device) : device{ device } {}
		auto addBinding(
			uint32_t binding,
			VkDescriptorType descriptorType,	// type
			VkShaderStageFlags stageFlags,		// which stages of pipeline have access is in flags
			uint32_t count = 1
		) -> Builder&;
		auto build() const->std::unique_ptr<DescriptorSetLayout>;
	};

	DescriptorSetLayout(Device& device, std::unordered_map<uint32_t, VkDescriptorSetLayoutBinding> bindings);
	~DescriptorSetLayout();
	DescriptorSetLayout(const DescriptorSetLayout&) = delete;
	auto operator=(const DescriptorSetLayout&)->DescriptorSetLayout & = delete;

	auto getDescriptorSetLayout() const -> VkDescriptorSetLayout { return this->descriptorSetLayout; }
};

export class DescriptorPool {
	Device& device;
	VkDescriptorPool descriptorPool;

	friend class DescriptorWriter;

public:
	class Builder {
		Device& device;
		std::vector<VkDescriptorPoolSize> poolSizes{};
		uint32_t maxSets = 1000; // total number of descriptor sets
		VkDescriptorPoolCreateFlags poolFlags = 0;

	public:
		Builder(Device& device) : device{ device } {}
		auto addPoolSize(VkDescriptorType descriptorType, uint32_t count) -> Builder&;
		auto setPoolFlags(VkDescriptorPoolCreateFlags flags) -> Builder&;
		auto setMaxSets(uint32_t count) -> Builder&;
		auto build() const->std::unique_ptr<DescriptorPool>;
	};

	DescriptorPool(
		Device& device,
		uint32_t maxSets,
		VkDescriptorPoolCreateFlags poolFlags,
		const std::vector<VkDescriptorPoolSize>& poolSizes
	);
	~DescriptorPool();
	DescriptorPool(const DescriptorPool&) = delete;
	auto operator=(const DescriptorPool&)->DescriptorPool & = delete;

	auto allocateDescriptor(const VkDescriptorSetLayout descriptorSetLayout, VkDescriptorSet& descriptor) const -> bool;
	auto freeDescriptors(std::vector<VkDescriptorSet>& descriptors) const -> void;
	auto resetPool() -> void;
};

export class DescriptorWriter {
	DescriptorSetLayout& setLayout;
	DescriptorPool& pool;
	std::vector<VkWriteDescriptorSet> writes;

public:
	DescriptorWriter(DescriptorSetLayout& setLayout, DescriptorPool& pool);

	auto writeBuffer(uint32_t binding, VkDescriptorBufferInfo* bufferInfo) -> DescriptorWriter&;
	auto writeImage(uint32_t binding, VkDescriptorImageInfo* imageInfo) -> DescriptorWriter&;

	auto build(VkDescriptorSet& set) -> bool;
	auto overwrite(VkDescriptorSet& set) -> void;
};

