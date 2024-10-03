
#include "Pipeline.hpp"

Pipeline::Pipeline(Device& device, size_t shaderModuleCount) :
	device{ device },
	shaderModules(shaderModuleCount)
{}
Pipeline::~Pipeline() {
	for (auto& shaderModule : this->shaderModules)
		vkDestroyShaderModule(this->device.device(), shaderModule, nullptr);
	vkDestroyPipeline(this->device.device(), this->pipeline, nullptr);
}

auto Pipeline::readFile(const std::string& filepath) -> std::vector<char> {
	std::ifstream file{ filepath, std::ios::ate | std::ios::binary }; // jump to end and as binary
	if (!file.is_open()) {
		throw std::runtime_error("Failed to open file: " + filepath);
	}
	size_t fileSize = static_cast<size_t>(file.tellg());
	std::vector<char> buffer(fileSize);

	file.seekg(0);
	file.read(buffer.data(), fileSize);
	file.close();
	return buffer;
}
auto Pipeline::createShaderModule(
	const std::vector<char>& code,
	VkShaderModule* shaderModule
) -> void {
	VkShaderModuleCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	createInfo.codeSize = code.size();
	createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data()); // default allocator in std::vector handles size mismatch issue

	if (vkCreateShaderModule(this->device.device(), &createInfo, nullptr, shaderModule) != VK_SUCCESS) {
		throw std::runtime_error("failed to create shader module.");
	}
}
auto Pipeline::bind(VkCommandBuffer commandBuffer, VkPipelineBindPoint bindPoint) -> void {
	vkCmdBindPipeline(
		commandBuffer,
		bindPoint,		// VK_PIPELINE_BIND_POINT_GRAPHICS, VK_PIPELINE_BIND_POINT_COMPUTE, VK_PIPELINE_BIND_POINT_RAY_TRACING
		this->pipeline
	);
}
