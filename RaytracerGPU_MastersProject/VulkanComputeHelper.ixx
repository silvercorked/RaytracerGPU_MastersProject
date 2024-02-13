module;

#include <iostream>
#include <fstream>
#include <unordered_set>
#include <set>
#include <array>
#include <random>
#include <chrono>

//#include <vulkan/vulkan.h>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

export module VulkanComputeHelper;

import PrimitiveTypes;
import Bitmap;

constexpr const u32 SAMPLE_COUNT = 1024*1024*16;

// local callback functions. not part of any class
static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
	VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
	VkDebugUtilsMessageTypeFlagsEXT messageType,
	const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
	void* pUserData
) {
	std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;
	return VK_FALSE;
}

struct QueueFamilyIndices {
	uint32_t graphicsAndComputeFamily;
	uint32_t Family;
	bool graphicsAndComputeFamilyHasValue = false;
	bool isComplete() { return graphicsAndComputeFamilyHasValue; }
};

struct UniformBufferObject {
	float iteration; // prob not needed for this case but might as well get some practice in
};

struct Logistic { // f(x; r) = xr(1-x)
	float x; // 2 ssbo's are used to store all these.
	float r; // one is input, the other is output. output's.x property will have the result from the previous frame.
};

export class VulkanComputeHelper {
	// createInstance
	VkInstance instance;
	const std::vector<const char*> validationLayers = { "VK_LAYER_KHRONOS_validation" };
	
	// setupDebugMessenger
	VkDebugUtilsMessengerEXT debugMessenger;

	// pickPhysicalDevice
	VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
	VkPhysicalDeviceProperties properties;

	// createLogicalDevice
	VkDevice device;
	const std::vector<const char*> deviceExtensions = {}; // { VK_KHR_SWAPCHAIN_EXTENSION_NAME };
	VkQueue GCQueue; // graphics and compute queue

	// createComputeDescriptorSetLayout
	VkDescriptorSetLayout computeDescriptorSetLayout;

	// createComputePipeline
	VkShaderModule computeShaderModule;
	VkPipelineShaderStageCreateInfo computeShaderStageInfo;
	VkPipeline computePipeline;
	VkPipelineLayout computePipelineLayout;

	// createCommandPool
	VkCommandPool commandPool;

	// createShaderStorageBuffers
	std::vector<VkBuffer> shaderStorageBuffers;				// strictly 2 for this case. no swapchain so just the ones i need
	std::vector<VkDeviceMemory> shaderStorageBuffersMemory;	// which are the prev and current

	// createUniformBuffers
	VkBuffer uniformBuffer;
	VkDeviceMemory uniformBufferMemory;
	void* uniformBufferMapped;

	// createDesciptorPool
	VkDescriptorPool descriptorPool;

	// createComputeDescriptorSets
	std::vector<VkDescriptorSet> computeDescriptorSets;

	// createComputeCommandBuffers
	VkCommandBuffer computeCommandBuffer;

	// createSyncObjects
	VkSemaphore computeFinishedSemaphore;
	VkFence computeInFlightFence;

	// mainLoop -> doIteration
	u32 iteration;

	auto initVulkan() -> void {
		this->createInstance();
		this->setupDebugMessenger();
		// this->createSurface();
		this->pickPhysicalDevice();
		this->createLogicalDevice();
		// this->createSwapChain();
		// this->createImageViews();
		// this->createRenderPass();
		this->createComputeDescriptorSetLayout();
		this->createComputePipeline();
		// this->createFrameBuffers();
		this->createCommandPool();
		this->createShaderStorageBuffers();
		this->createUniformBuffers();
		this->createDescriptorPool();
		this->createComputeDescriptorSets();
		this->createComputeCommandBuffers();
		this->createSyncObjects();
	}
	auto createInstance() -> void;
	auto getRequiredExtensions() -> std::vector<const char*>;
	auto populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT&) -> void;
	auto hasGflwRequiredInstanceExtensions() -> void;
	
	auto setupDebugMessenger() -> void;
	auto CreateDebugUtilsMessengerEXT(
		VkInstance,
		const VkDebugUtilsMessengerCreateInfoEXT*,
		const VkAllocationCallbacks*,
		VkDebugUtilsMessengerEXT*
	) -> VkResult;
	auto DestroyDebugUtilsMessengerEXT(
		VkInstance,
		VkDebugUtilsMessengerEXT,
		const VkAllocationCallbacks*
	) -> void;
	
	auto pickPhysicalDevice() -> void;
	auto isDeviceSuitable(VkPhysicalDevice) -> bool;
	auto findQueueFamilies(VkPhysicalDevice) -> QueueFamilyIndices;

	auto createLogicalDevice() -> void;

	auto createComputeDescriptorSetLayout() -> void;

	auto createComputePipeline() -> void;
	auto createShaderModule(const std::vector<char>&) -> VkShaderModule;
	auto readFile(const std::string&) -> std::vector<char>;

	auto createCommandPool() -> void;

	auto createShaderStorageBuffers() -> void;
	auto createBuffer(
		VkDeviceSize,
		VkBufferUsageFlags,
		VkMemoryPropertyFlags,
		VkBuffer&,
		VkDeviceMemory&
	) -> void;
	auto copyBuffer(VkBuffer, VkBuffer, VkDeviceSize) -> void;
	auto findMemoryType(u32, VkMemoryPropertyFlags) -> u32;

	auto createUniformBuffers() -> void;

	auto createDescriptorPool() -> void;

	auto createComputeDescriptorSets() -> void;

	auto createComputeCommandBuffers() -> void;

	auto createSyncObjects() -> void;

	auto doIteration() -> void {
		std::cout << "doing iteration" << std::endl;
		VkSubmitInfo submitInfo{};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

		vkWaitForFences(this->device, 1, &this->computeInFlightFence, VK_TRUE, UINT64_MAX); // await compute completion

		auto currentTime = std::chrono::high_resolution_clock::now();

		// read computed result
		{
			VkBuffer stagingBuffer;
			VkDeviceMemory stagingBufferMemory;
			const auto bufferSize = sizeof(Logistic) * SAMPLE_COUNT;

			this->createBuffer(
				bufferSize,
				VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
				stagingBuffer,
				stagingBufferMemory
			);

			void* data;
			vkMapMemory(this->device, stagingBufferMemory, 0, bufferSize, 0, &data);

			this->copyBuffer(this->shaderStorageBuffers[this->iteration % 2], stagingBuffer, bufferSize);

			vkUnmapMemory(this->device, stagingBufferMemory);
			// data has stuff now?
			Logistic* logistics = reinterpret_cast<Logistic*>(data);
			std::vector<Logistic> resultFromGPU{};
			resultFromGPU.reserve(SAMPLE_COUNT);
			for (u32 i = 0; i < SAMPLE_COUNT; i++) {
				resultFromGPU.push_back(logistics[i]);
			}
			std::cout << "one result: \n\t"
				<< "x: " << resultFromGPU.at(0).x << " r: " << resultFromGPU.at(0).r << std::endl;
			const u32 imageSize = 10000;
			u32 weirdPixelCount = 0;
			Bitmap bmp(imageSize, imageSize, 255);
			for (u32 i = 0; i < resultFromGPU.size(); i++) {
				//if (this->iteration != 1 && std::abs(0.5f - resultFromGPU.at(i).x) <= 0.001f) {
				//	std::cout << "Good point at i=" << i <<
				//		" x:" << resultFromGPU.at(i).x <<
				//		" r:" << resultFromGPU.at(i).r << " size: " << resultFromGPU.size() << std::endl;
				//}
				bmp.setPixel(
					static_cast<u32>((resultFromGPU.at(i).r - 3.5f) * (static_cast<float>(imageSize - 1) * 2.0f)),
					imageSize - static_cast<u32>(resultFromGPU.at(i).x * static_cast<float>(imageSize - 1)),
					Bitmap::RED
				);
			}
			bmp.saveBitmap("logistic_gpu_" + std::to_string(this->iteration) + "_iterations.bmp");

			vkDestroyBuffer(this->device, stagingBuffer, nullptr);
			vkFreeMemory(this->device, stagingBufferMemory, nullptr);
		}
		// end read computed result

		auto newTime = std::chrono::high_resolution_clock::now();
		float frameTime = std::chrono::duration<float, std::chrono::milliseconds::period>(newTime - currentTime).count();
		currentTime = newTime;
		std::cout << "copy from gpu Time: " << frameTime << std::endl;

		vkResetFences(this->device, 1, &this->computeInFlightFence); // reset for next run

		vkResetCommandBuffer(this->computeCommandBuffer, 0);
		this->recordComputeCommandBuffer(this->computeCommandBuffer);

		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &this->computeCommandBuffer;
		submitInfo.signalSemaphoreCount = 0;
		//submitInfo.pSignalSemaphores = &this->computeFinishedSemaphore; // think this is only needed if compute needs to wait on something else

		if (vkQueueSubmit(this->GCQueue, 1, &submitInfo, this->computeInFlightFence) != VK_SUCCESS) {
			throw std::runtime_error("failed to submit compute command buffer!");
		}
		this->iteration++;
	}

	auto recordComputeCommandBuffer(VkCommandBuffer) -> void;

public:
	VulkanComputeHelper();
	auto mainLoop() -> void {
		auto currentTime = std::chrono::high_resolution_clock::now();
		while (true) {
			if (this->iteration > 102) break;
			auto newTime = std::chrono::high_resolution_clock::now();
			float frameTime = std::chrono::duration<float, std::chrono::milliseconds::period>(newTime - currentTime).count();
			currentTime = newTime;
			std::cout << "Iteration Time: " << frameTime << std::endl;
			doIteration();
		}
		vkDeviceWaitIdle(this->device);
	}
	~VulkanComputeHelper();
};

VulkanComputeHelper::VulkanComputeHelper() {
	this->initVulkan();
}
VulkanComputeHelper::~VulkanComputeHelper() {
	vkDestroyPipeline(device, computePipeline, nullptr);
	vkDestroyPipelineLayout(device, computePipelineLayout, nullptr);

	vkDestroyBuffer(device, uniformBuffer, nullptr);
	vkFreeMemory(device, uniformBufferMemory, nullptr);

	vkDestroyDescriptorPool(this->device, this->descriptorPool, nullptr);
	vkDestroyDescriptorSetLayout(this->device, this->computeDescriptorSetLayout, nullptr);

	for (size_t i = 0; i < 2; i++) {
		vkDestroyBuffer(this->device, this->shaderStorageBuffers[i], nullptr);
		vkFreeMemory(this->device, this->shaderStorageBuffersMemory[i], nullptr);
	}

	vkDestroySemaphore(this->device, this->computeFinishedSemaphore, nullptr);
	vkDestroyFence(this->device, this->computeInFlightFence, nullptr);

	vkDestroyCommandPool(this->device, this->commandPool, nullptr);

	vkDestroyDevice(this->device, nullptr);
	this->DestroyDebugUtilsMessengerEXT(this->instance, this->debugMessenger, nullptr);

	vkDestroyInstance(this->instance, nullptr);
}

auto VulkanComputeHelper::createInstance() -> void {
	VkApplicationInfo appInfo{};
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pApplicationName = "Hello Triangle";
	appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.pEngineName = "No Engine";
	appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.apiVersion = VK_API_VERSION_1_0;

	VkInstanceCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	createInfo.pApplicationInfo = &appInfo;

	auto extensions = getRequiredExtensions();
	createInfo.enabledExtensionCount = static_cast<u32>(extensions.size());
	createInfo.ppEnabledExtensionNames = extensions.data();

	VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo;
	{ // debugging assumed
		createInfo.enabledLayerCount = static_cast<u32>(this->validationLayers.size());
		createInfo.ppEnabledLayerNames = this->validationLayers.data();

		populateDebugMessengerCreateInfo(debugCreateInfo);
		createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debugCreateInfo;
	}
	/*
		if not assuming debugging
		createInfo.enabledLayerCount = 0;
		createInfo.pNext = nullptr;
	*/
	if (vkCreateInstance(&createInfo, nullptr, &this->instance) != VK_SUCCESS)
		throw std::runtime_error("Failed to Create Instance!");

	// hasGflwRequiredInstanceExtensions(); // validation step
}
auto VulkanComputeHelper::getRequiredExtensions() -> std::vector<const char*> {
	/*
	* // disabling these as i don't believe glfw is necessary for compute work alone
	uint32_t glfwExtensionCount = 0;
	const char** glfwExtensions;
	glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

	std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);
	*/
	std::vector<const char*> extensions{};
	extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME); // debugging assumed

	return extensions;
}
auto VulkanComputeHelper::populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo) -> void {
	createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
	createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
	createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
	createInfo.pfnUserCallback = debugCallback;
	createInfo.pUserData = nullptr;  // Optional
}
void VulkanComputeHelper::hasGflwRequiredInstanceExtensions() {
	uint32_t extensionCount = 0;
	vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
	std::vector<VkExtensionProperties> extensions(extensionCount);
	vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensions.data());

	std::cout << "available extensions:" << std::endl;
	std::unordered_set<std::string> available;
	for (const auto& extension : extensions) {
		std::cout << "\t" << extension.extensionName << std::endl;
		available.insert(extension.extensionName);
	}

	std::cout << "required extensions:" << std::endl;
	auto requiredExtensions = this->getRequiredExtensions();
	for (const auto& required : requiredExtensions) {
		std::cout << "\t" << required << std::endl;
		if (available.find(required) == available.end()) {
			throw std::runtime_error("Missing required glfw extension");
		}
	}
}

auto VulkanComputeHelper::setupDebugMessenger() -> void {
	// debug assumed
	VkDebugUtilsMessengerCreateInfoEXT createInfo;
	populateDebugMessengerCreateInfo(createInfo);
	if (CreateDebugUtilsMessengerEXT(this->instance, &createInfo, nullptr, &this->debugMessenger) != VK_SUCCESS) {
		throw std::runtime_error("failed to set up debug messenger!");
	}
}
VkResult VulkanComputeHelper::CreateDebugUtilsMessengerEXT(
	VkInstance instance,
	const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
	const VkAllocationCallbacks* pAllocator,
	VkDebugUtilsMessengerEXT* pDebugMessenger
) {
	auto func = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(
		instance,
		"vkCreateDebugUtilsMessengerEXT"
	);
	if (func != nullptr) {
		return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
	}
	else {
		return VK_ERROR_EXTENSION_NOT_PRESENT;
	}
}
void VulkanComputeHelper::DestroyDebugUtilsMessengerEXT(
	VkInstance instance,
	VkDebugUtilsMessengerEXT debugMessenger,
	const VkAllocationCallbacks* pAllocator
) {
	auto func = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(
		instance,
		"vkDestroyDebugUtilsMessengerEXT"
	);
	if (func != nullptr) {
		func(instance, debugMessenger, pAllocator);
	}
}

void VulkanComputeHelper::pickPhysicalDevice() {
	uint32_t deviceCount = 0;
	vkEnumeratePhysicalDevices(this->instance, &deviceCount, nullptr);
	if (deviceCount == 0) {
		throw std::runtime_error("failed to find GPUs with Vulkan support!");
	}
	std::cout << "Device count: " << deviceCount << std::endl;
	std::vector<VkPhysicalDevice> devices(deviceCount);
	vkEnumeratePhysicalDevices(this->instance, &deviceCount, devices.data());

	for (const auto& device : devices) {
		if (isDeviceSuitable(device)) { // prob improvement is rating devices and selecting the best based on features available
			this->physicalDevice = device;
			break;
		}
	}

	if (this->physicalDevice == VK_NULL_HANDLE) {
		throw std::runtime_error("failed to find a suitable GPU!");
	}

	vkGetPhysicalDeviceProperties(this->physicalDevice, &this->properties);
	std::cout << "physical device: " << this->properties.deviceName << std::endl;
}
auto VulkanComputeHelper::isDeviceSuitable(VkPhysicalDevice device) -> bool {
	QueueFamilyIndices indices = this->findQueueFamilies(device);

	return indices.isComplete();
}
QueueFamilyIndices VulkanComputeHelper::findQueueFamilies(VkPhysicalDevice device) {
	QueueFamilyIndices indices;

	uint32_t queueFamilyCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

	std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

	int i = 0; // need compute queue and graphics queue (no present needed yet. maybe graphics unneeded here).
	for (const auto& queueFamily : queueFamilies) {
		if (
			queueFamily.queueCount > 0
			&& (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
			&& (queueFamily.queueFlags & VK_QUEUE_COMPUTE_BIT)
		) {
			indices.graphicsAndComputeFamily = i;
			indices.graphicsAndComputeFamilyHasValue = true;
		}
		if (indices.isComplete()) {
			break;
		}

		i++;
	}

	return indices;
}

auto VulkanComputeHelper::createLogicalDevice() -> void {
	QueueFamilyIndices indices = this->findQueueFamilies(this->physicalDevice);

	std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
	std::set<uint32_t> uniqueQueueFamilies = { indices.graphicsAndComputeFamily };

	float queuePriority = 1.0f;
	for (uint32_t queueFamily : uniqueQueueFamilies) {
		VkDeviceQueueCreateInfo queueCreateInfo = {};
		queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queueCreateInfo.queueFamilyIndex = queueFamily;
		queueCreateInfo.queueCount = 1;
		queueCreateInfo.pQueuePriorities = &queuePriority;
		queueCreateInfos.push_back(queueCreateInfo);
	}

	VkPhysicalDeviceFeatures deviceFeatures = {};
	deviceFeatures.samplerAnisotropy = VK_TRUE;

	VkDeviceCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

	createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
	createInfo.pQueueCreateInfos = queueCreateInfos.data();

	createInfo.pEnabledFeatures = &deviceFeatures;
	createInfo.enabledExtensionCount = static_cast<uint32_t>(this->deviceExtensions.size());
	createInfo.ppEnabledExtensionNames = this->deviceExtensions.data();

	// might not really be necessary anymore because device specific validation layers
	// have been deprecated
	// debug assumed
	{
		createInfo.enabledLayerCount = static_cast<uint32_t>(this->validationLayers.size());
		createInfo.ppEnabledLayerNames = this->validationLayers.data();
	}

	if (vkCreateDevice(this->physicalDevice, &createInfo, nullptr, &this->device) != VK_SUCCESS) {
		throw std::runtime_error("failed to create logical device!");
	}

	vkGetDeviceQueue(this->device, indices.graphicsAndComputeFamily, 0, &this->GCQueue);
}

auto VulkanComputeHelper::createComputeDescriptorSetLayout() -> void {
	std::array<VkDescriptorSetLayoutBinding, 3> layoutBindings{};
	layoutBindings[0].binding = 0;
	layoutBindings[0].descriptorCount = 1;
	layoutBindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	layoutBindings[0].pImmutableSamplers = nullptr;
	layoutBindings[0].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

	layoutBindings[1].binding = 1;
	layoutBindings[1].descriptorCount = 1;
	layoutBindings[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	layoutBindings[1].pImmutableSamplers = nullptr;
	layoutBindings[1].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

	layoutBindings[2].binding = 2;
	layoutBindings[2].descriptorCount = 1;
	layoutBindings[2].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	layoutBindings[2].pImmutableSamplers = nullptr;
	layoutBindings[2].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

	VkDescriptorSetLayoutCreateInfo layoutInfo{};
	layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutInfo.bindingCount = 3;
	layoutInfo.pBindings = layoutBindings.data();

	if (vkCreateDescriptorSetLayout(this->device, &layoutInfo, nullptr, &this->computeDescriptorSetLayout) != VK_SUCCESS) {
		throw std::runtime_error("failed to create compute descriptor set layout!");
	}
}

auto VulkanComputeHelper::createComputePipeline() -> void {
	auto computeShaderCode = readFile("shaders/compiled/logistic.comp.spv");

	this->computeShaderModule = this->createShaderModule(computeShaderCode);

	this->computeShaderStageInfo = {};
	this->computeShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	this->computeShaderStageInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
	this->computeShaderStageInfo.module = this->computeShaderModule;
	this->computeShaderStageInfo.pName = "main"; // function name to invoke in compute shader. main is prob good to always stick too. least for me rn

	VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = 1;
	pipelineLayoutInfo.pSetLayouts = &this->computeDescriptorSetLayout;

	if (vkCreatePipelineLayout(this->device, &pipelineLayoutInfo, nullptr, &this->computePipelineLayout) != VK_SUCCESS)
		throw std::runtime_error("failed to create compute pipeline layout!");

	VkComputePipelineCreateInfo pipelineInfo{};
	pipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
	pipelineInfo.layout = this->computePipelineLayout;
	pipelineInfo.stage = this->computeShaderStageInfo;

	if (vkCreateComputePipelines(this->device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &this->computePipeline) != VK_SUCCESS)
		throw std::runtime_error("failed to create compute pipeline!");

	vkDestroyShaderModule(this->device, this->computeShaderModule, nullptr);
}
auto VulkanComputeHelper::createShaderModule(const std::vector<char>& code) -> VkShaderModule {
	VkShaderModuleCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	createInfo.codeSize = code.size();
	createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());
	VkShaderModule shaderModule;
	if (vkCreateShaderModule(this->device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
		throw std::runtime_error("failed to create shader module!");
	}
	return shaderModule;
}
auto VulkanComputeHelper::readFile(const std::string& filepath) -> std::vector<char> { // needs <fstream>
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

auto VulkanComputeHelper::createCommandPool() -> void {
	QueueFamilyIndices queueFamilyIndices = this->findQueueFamilies(physicalDevice);

	VkCommandPoolCreateInfo poolInfo{};
	poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsAndComputeFamily;

	if (vkCreateCommandPool(this->device, &poolInfo, nullptr, &this->commandPool) != VK_SUCCESS)
		throw std::runtime_error("failed to create graphics command pool!");
}

auto VulkanComputeHelper::createShaderStorageBuffers() -> void {
	// initialize logistic sample points
	std::default_random_engine rndEngine((unsigned)time(nullptr));
	std::uniform_real_distribution<float> rndDist(0.0f, 1.0f); // parameter r is 0-4, x input is 0-1

	std::vector<Logistic> points(SAMPLE_COUNT);
	for (auto& point : points) {
		point.r = 3.5f + (rndDist(rndEngine) / 2.0f);			// r: 3.5-4
		point.x = rndDist(rndEngine);					// x: 0-1
	}

	VkDeviceSize bufferSize = sizeof(Logistic) * SAMPLE_COUNT;

	// Create Staging buffer to upload data to gpu
	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;
	this->createBuffer(
		bufferSize,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		stagingBuffer,
		stagingBufferMemory
	);

	void* data;
	vkMapMemory(this->device, stagingBufferMemory, 0, bufferSize, 0, &data);
	memcpy(data, points.data(), static_cast<size_t>(bufferSize));
	vkUnmapMemory(this->device, stagingBufferMemory);

	this->shaderStorageBuffers.resize(2);
	this->shaderStorageBuffersMemory.resize(2);

	// copy init data to both buffers
	for (size_t i = 0; i < 2; i++) {
		this->createBuffer( // create ssbo buffers on the gpu that can be transfered to from the cpu
			bufferSize,
			VK_BUFFER_USAGE_STORAGE_BUFFER_BIT // is SSBO
			| VK_BUFFER_USAGE_TRANSFER_DST_BIT // can transfer to from cpu
			| VK_BUFFER_USAGE_TRANSFER_SRC_BIT, // can read from into cpu
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			this->shaderStorageBuffers[i],
			this->shaderStorageBuffersMemory[i]
		);
		this->copyBuffer(stagingBuffer, this->shaderStorageBuffers[i], bufferSize);
	}

	vkDestroyBuffer(this->device, stagingBuffer, nullptr);
	vkFreeMemory(this->device, stagingBufferMemory, nullptr);
}
auto VulkanComputeHelper::createBuffer(
	VkDeviceSize size,
	VkBufferUsageFlags usage,
	VkMemoryPropertyFlags properties,
	VkBuffer& buffer,
	VkDeviceMemory& bufferMemory
) -> void {
	VkBufferCreateInfo bufferInfo{};
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.size = size;
	bufferInfo.usage = usage;
	bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	if (vkCreateBuffer(this->device, &bufferInfo, nullptr, &buffer) != VK_SUCCESS)
		throw std::runtime_error("failed to create buffer!");

	VkMemoryRequirements memRequirements;
	vkGetBufferMemoryRequirements(this->device, buffer, &memRequirements);

	VkMemoryAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memRequirements.size;
	allocInfo.memoryTypeIndex = this->findMemoryType(memRequirements.memoryTypeBits, properties);

	if (vkAllocateMemory(this->device, &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS)
		throw std::runtime_error("failed to allocate buffer memory!");

	vkBindBufferMemory(this->device, buffer, bufferMemory, 0);
}
auto VulkanComputeHelper::copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size) -> void {
	VkCommandBufferAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandPool = this->commandPool;
	allocInfo.commandBufferCount = 1;

	VkCommandBuffer commandBuffer;
	vkAllocateCommandBuffers(this->device, &allocInfo, &commandBuffer);

	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	vkBeginCommandBuffer(commandBuffer, &beginInfo);

	VkBufferCopy copyRegion{};
	copyRegion.size = size;
	vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

	vkEndCommandBuffer(commandBuffer);

	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffer;

	vkQueueSubmit(this->GCQueue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(this->GCQueue);

    vkFreeCommandBuffers(this->device, this->commandPool, 1, &commandBuffer);
}
auto VulkanComputeHelper::findMemoryType(u32 typeFilter, VkMemoryPropertyFlags properties) -> u32 {
	VkPhysicalDeviceMemoryProperties memProperties;
	vkGetPhysicalDeviceMemoryProperties(this->physicalDevice, &memProperties);

	for (u32 i = 0; i < memProperties.memoryTypeCount; i++) {
		if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
			return i;
		}
	}
	throw std::runtime_error("failed to find suitable memory type!");
}

auto VulkanComputeHelper::createUniformBuffers() -> void { // just 1
	VkDeviceSize bufferSize = sizeof(UniformBufferObject);
	this->createBuffer(
		bufferSize,
		VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
		| VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		this->uniformBuffer,
		this->uniformBufferMemory
	);
	vkMapMemory(this->device, this->uniformBufferMemory, 0, bufferSize, 0, &this->uniformBufferMapped);
}

auto VulkanComputeHelper::createDescriptorPool() -> void {
	std::array<VkDescriptorPoolSize, 2> poolSizes{};
	poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	poolSizes[0].descriptorCount = static_cast<u32>(2);
	poolSizes[1].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	poolSizes[1].descriptorCount = static_cast<u32>(2);

	VkDescriptorPoolCreateInfo poolInfo{};
	poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolInfo.poolSizeCount = 2;
	poolInfo.pPoolSizes = poolSizes.data();
	poolInfo.maxSets = 2; // need 2 descriptor sets. see createComputeDescriptorSets

	if (vkCreateDescriptorPool(this->device, &poolInfo, nullptr, &this->descriptorPool) != VK_SUCCESS)
		throw std::runtime_error("Failed to create descriptor pool!");
}

auto VulkanComputeHelper::createComputeDescriptorSets() -> void {
	std::vector<VkDescriptorSetLayout> layouts(2, this->computeDescriptorSetLayout);
	VkDescriptorSetAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = this->descriptorPool;
	allocInfo.descriptorSetCount = static_cast<u32>(2);
	allocInfo.pSetLayouts = layouts.data();

	this->computeDescriptorSets.resize(2);
	if (vkAllocateDescriptorSets(this->device, &allocInfo, this->computeDescriptorSets.data()) != VK_SUCCESS) {
		throw std::runtime_error("Failed to allocate descriptor sets!");
	}

	// need to have 2 descriptor sets cause in 1 run, it's the first buffer as input, then in the other its the second
	for (u32 i = 0; i < 2; i++) {
		VkDescriptorBufferInfo uniformBufferInfo{};
		uniformBufferInfo.buffer = this->uniformBuffer;
		uniformBufferInfo.offset = 0;
		uniformBufferInfo.range = sizeof(UniformBufferObject);

		std::array<VkWriteDescriptorSet, 3> descriptorWrites{};
		descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[0].dstSet = this->computeDescriptorSets[i];
		descriptorWrites[0].dstBinding = 0;
		descriptorWrites[0].dstArrayElement = 0;
		descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		descriptorWrites[0].descriptorCount = 1;
		descriptorWrites[0].pBufferInfo = &uniformBufferInfo;

		VkDescriptorBufferInfo storageBufferInfoPrev{};
		storageBufferInfoPrev.buffer = this->shaderStorageBuffers[(i - 1) % 2]; // prev writes to current, current writes to prev
		storageBufferInfoPrev.offset = 0;
		storageBufferInfoPrev.range = sizeof(Logistic) * SAMPLE_COUNT;

		descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[1].dstSet = this->computeDescriptorSets[i];
		descriptorWrites[1].dstBinding = 1;
		descriptorWrites[1].dstArrayElement = 0;
		descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		descriptorWrites[1].descriptorCount = 1;
		descriptorWrites[1].pBufferInfo = &storageBufferInfoPrev;

		VkDescriptorBufferInfo storageBufferInfoCurrentFrame{};
		storageBufferInfoCurrentFrame.buffer = this->shaderStorageBuffers[i];
		storageBufferInfoCurrentFrame.offset = 0;
		storageBufferInfoCurrentFrame.range = sizeof(Logistic) * SAMPLE_COUNT;

		descriptorWrites[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[2].dstSet = this->computeDescriptorSets[i];
		descriptorWrites[2].dstBinding = 2;
		descriptorWrites[2].dstArrayElement = 0;
		descriptorWrites[2].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		descriptorWrites[2].descriptorCount = 1;
		descriptorWrites[2].pBufferInfo = &storageBufferInfoCurrentFrame;

		vkUpdateDescriptorSets(device, 3, descriptorWrites.data(), 0, nullptr);
	}
}

auto VulkanComputeHelper::createComputeCommandBuffers() -> void {
	VkCommandBufferAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.commandPool = this->commandPool;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandBufferCount = static_cast<u32>(1);

	if (vkAllocateCommandBuffers(this->device, &allocInfo, &this->computeCommandBuffer) != VK_SUCCESS)
		throw std::runtime_error("Failed to allocate compute Command Buffers!");
}

auto VulkanComputeHelper::createSyncObjects() -> void {
	VkSemaphoreCreateInfo semaphoreInfo{};
	semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	VkFenceCreateInfo fenceInfo{};
	fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

	if (
		vkCreateSemaphore(this->device, &semaphoreInfo, nullptr, &this->computeFinishedSemaphore) != VK_SUCCESS
		|| vkCreateFence(this->device, &fenceInfo, nullptr, &computeInFlightFence) != VK_SUCCESS
	)
		throw std::runtime_error("Failed to create compute synchronization object!");
}

void VulkanComputeHelper::recordComputeCommandBuffer(VkCommandBuffer commandBuffer) {
	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

	if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
		throw std::runtime_error("failed to begin recording compute command buffer!");
	}

	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, this->computePipeline);

	vkCmdBindDescriptorSets(
		commandBuffer,
		VK_PIPELINE_BIND_POINT_COMPUTE,
		this->computePipelineLayout,
		0,
		1,
		&this->computeDescriptorSets[this->iteration % 2],
		0,
		nullptr
	);

	vkCmdDispatch(commandBuffer, SAMPLE_COUNT / 1024, 1, 1);

	if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
		throw std::runtime_error("failed to record compute command buffer!");
	}
}
