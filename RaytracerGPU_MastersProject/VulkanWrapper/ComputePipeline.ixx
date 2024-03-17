module;

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <cassert>

export module VulkanWrap:ComputePipeline;

import :Device;
import :Model;
import :Pipeline;

import <string>;
import <vector>;
import <fstream>;
import <stdexcept>;
import <iostream>;

export struct ComputePipelineConfigInfo {

	ComputePipelineConfigInfo() = default;

	ComputePipelineConfigInfo(const ComputePipelineConfigInfo&) = delete;
	ComputePipelineConfigInfo& operator=(const ComputePipelineConfigInfo&) = delete;

	VkPipelineCreateFlags							createFlags;
	VkPipelineLayout								pipelineLayout;
	uint32_t										subpass;
};

export class ComputePipeline : protected Pipeline {

	auto createComputePipeline(const std::string& computeFilepath, const ComputePipelineConfigInfo& config) -> void;
	auto getComputeShaderModule() -> VkShaderModule& { return this->shaderModules[0]; }
public:
	ComputePipeline(Device& device, const std::string& computeFilepath, const ComputePipelineConfigInfo& config);
	~ComputePipeline();

	ComputePipeline(const ComputePipeline&) = delete;
	ComputePipeline& operator=(const ComputePipeline&) = delete;

	auto bind(VkCommandBuffer commandBuffer) -> void override;

	static auto defaultPipelineConfigInfo(ComputePipelineConfigInfo& configInfo) -> void;
};
