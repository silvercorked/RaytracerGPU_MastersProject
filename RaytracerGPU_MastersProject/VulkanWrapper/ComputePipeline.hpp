#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "../utils/PrimitiveTypes.hpp"

#include "Device.hpp"
#include "abstract/Pipeline.hpp"

#include <string>
#include <vector>

struct ComputePipelineConfigInfo {

	ComputePipelineConfigInfo() = default;

	ComputePipelineConfigInfo(const ComputePipelineConfigInfo&) = delete;
	ComputePipelineConfigInfo& operator=(const ComputePipelineConfigInfo&) = delete;

	VkPipelineCreateFlags							createFlags;
	VkPipelineLayout								pipelineLayout;
	uint32_t										subpass;
};

class ComputePipeline : protected Pipeline {

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
