module;

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <cassert>

export module VulkanWrap:Pipeline;

import PrimitiveTypes;

import :Device;
import :Model;

import <string>;
import <vector>;
import <fstream>;
import <stdexcept>;
import <iostream>;

export class Pipeline {

protected:
	Device& device;
	VkPipeline pipeline;
	std::vector<VkShaderModule> shaderModules;

	auto createShaderModule(const std::vector<char>& code, VkShaderModule* shaderModule) -> void;
	auto bind(VkCommandBuffer commandBuffer, VkPipelineBindPoint bindPoint) -> void;

	static auto readFile(const std::string& filepath) -> std::vector<char>;
public:
	Pipeline(Device& device, size_t shaderModuleCount = 1);
	~Pipeline();

	Pipeline() = delete;
	Pipeline(const Pipeline&) = delete;
	Pipeline& operator=(const Pipeline&) = delete;

	virtual auto bind(VkCommandBuffer commandBuffer) -> void = 0;
};
