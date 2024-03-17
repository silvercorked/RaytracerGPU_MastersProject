
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <cassert>

module VulkanWrap:ComputePipeline;

ComputePipeline::ComputePipeline(
	Device& device,
	const std::string& computeFilepath,
	const ComputePipelineConfigInfo& config
) :
	Pipeline{ device, 1 }
{
	this->createComputePipeline(computeFilepath, config);
}
ComputePipeline::~ComputePipeline() {}

auto ComputePipeline::createComputePipeline(
	const std::string& computeFilepath,
	const ComputePipelineConfigInfo& config
) -> void {
	auto computeCode = this->readFile(computeFilepath);

	assert(
		config.pipelineLayout != VK_NULL_HANDLE &&
		"Cannot create compute pipeline: no pipelineLayout provided in configInfo"
	);
	this->createShaderModule(computeCode, &this->shaderModules[0]);

	VkPipelineShaderStageCreateInfo shaderStageInfo{};

	// setup vertex shader stage
	shaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	shaderStageInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;						// for compute shader
	shaderStageInfo.module = this->getComputeShaderModule();					// shader module
	shaderStageInfo.pName = "main";												// name of entry function in shader
	shaderStageInfo.flags = 0;													// unused
	shaderStageInfo.pNext = nullptr;											// for customizing shader functionality, unused								
	shaderStageInfo.pSpecializationInfo = nullptr;								// for customizing shader functionality, unused	

	// create pipeline object and connect to config
	VkComputePipelineCreateInfo pipelineInfo{};
	pipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
	pipelineInfo.pNext = nullptr;
	pipelineInfo.flags = config.createFlags;
	pipelineInfo.stage = shaderStageInfo;
	pipelineInfo.layout = config.pipelineLayout;

	pipelineInfo.basePipelineIndex = -1;				// used for performance, allows creation of pipeline by derivation of
	pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;	// an existing pipeline on the gpu

	if (
		vkCreateComputePipelines(
			this->device.device(),
			VK_NULL_HANDLE,				// pipeline cache, can have better performance
			1,							// single pipeline
			&pipelineInfo,				// 
			nullptr,					// no allocation callbacks
			&this->pipeline				// handle for this graphics pipeline
		) != VK_SUCCESS
		) {
		throw std::runtime_error("failed to create compute pipeline");
	}
}
auto ComputePipeline::bind(VkCommandBuffer commandBuffer) -> void {
	vkCmdBindPipeline(
		commandBuffer,
		VK_PIPELINE_BIND_POINT_COMPUTE,		// VK_PIPELINE_BIND_POINT_GRAPHICS, VK_PIPELINE_BIND_POINT_COMPUTE, VK_PIPELINE_BIND_POINT_RAY_TRACING
		this->pipeline
	);
}
auto ComputePipeline::defaultPipelineConfigInfo(ComputePipelineConfigInfo& configInfo) -> void {
	configInfo.subpass = 0;
	configInfo.createFlags = 0; // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkPipelineCreateFlagBits.html
	configInfo.pipelineLayout = nullptr;
}
