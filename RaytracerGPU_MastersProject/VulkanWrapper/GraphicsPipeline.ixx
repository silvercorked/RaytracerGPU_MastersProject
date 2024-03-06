module;

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <cassert>

export module VulkanWrap:GraphicsPipeline;

import :Device;
import :Model;
import :Pipeline;

import <string>;
import <vector>;
import <fstream>;
import <stdexcept>;
import <iostream>;

struct GraphicsPipelineConfigInfo {

	GraphicsPipelineConfigInfo() = default;

	GraphicsPipelineConfigInfo(const GraphicsPipelineConfigInfo&) = delete;
	GraphicsPipelineConfigInfo& operator=(const GraphicsPipelineConfigInfo&) = delete;

	std::vector<VkVertexInputBindingDescription>	bindingDescriptions{};
	std::vector<VkVertexInputAttributeDescription>	attributeDescriptions{};
	VkPipelineInputAssemblyStateCreateInfo			inputAssemblyInfo;
	VkPipelineViewportStateCreateInfo				viewportInfo;
	VkPipelineRasterizationStateCreateInfo			rasterizationInfo;
	VkPipelineMultisampleStateCreateInfo			multisampleInfo;
	VkPipelineColorBlendAttachmentState				colorBlendAttachement;
	VkPipelineColorBlendStateCreateInfo				colorBlendInfo;
	VkPipelineDepthStencilStateCreateInfo			depthStencilInfo;
	std::vector<VkDynamicState>						dynamicStateEnables;
	VkPipelineDynamicStateCreateInfo				dynamicStateInfo;
	VkPipelineLayout								pipelineLayout = nullptr;
	VkRenderPass									renderPass = nullptr;
	uint32_t										subpass = 0;
};

export class GraphicsPipeline : protected Pipeline {

	auto createGraphicsPipeline(const std::string& vertFilepath, const std::string& fragFilepath, const GraphicsPipelineConfigInfo& config) -> void;
	auto getVertShaderModule() -> VkShaderModule& { return this->shaderModules[0]; }
	auto getFragShaderModule() -> VkShaderModule& { return this->shaderModules[1]; } // works cause constructor guarentees these allocations
public:
	GraphicsPipeline(Device& device, const std::string& vertFilepath, const std::string& fragFilepath, const GraphicsPipelineConfigInfo& config);
	~GraphicsPipeline();

	GraphicsPipeline(const GraphicsPipeline&) = delete;
	GraphicsPipeline& operator=(const GraphicsPipeline&) = delete;

	auto bind(VkCommandBuffer commandBuffer) -> void override;

	static auto defaultPipelineConfigInfo(GraphicsPipelineConfigInfo& configInfo) -> void;
	static auto enableAlphaBlending(GraphicsPipelineConfigInfo& configInfo) -> void;
};
