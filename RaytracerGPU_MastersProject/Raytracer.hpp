#pragma once

//#include <vulkan/vulkan.h>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS					// functions expect radians, not degrees
#define GLM_FORCE_DEPTH_ZERO_TO_ONE			// Depth buffer values will range from 0 to 1, not -1 to 1
#include <glm/glm.hpp>

#include "utils/PrimitiveTypes.hpp"
#include "Config.hpp"
#include "VulkanWrapper/Window.hpp"
#include "VulkanWrapper/Device.hpp"
#include "VulkanWrapper/SwapChain.hpp"
#include "VulkanWrapper/ComputePipeline.hpp"
#include "VulkanWrapper/GraphicsPipeline.hpp"
#include "VulkanWrapper/Buffer.hpp"
#include "VulkanWrapper/Descriptors.hpp"

#include <chrono>
#include "VulkanWrapper/RaytraceScene.hpp"
#include <random>

#include <format>
#include <stdexcept>
#include "VulkanWrapper/SceneTypes.hpp"

namespace RaytracerRenderer {
	struct UniformBufferObject {
		alignas(16) glm::vec3 camPos;
		alignas(16) glm::vec3 camLookAt;
		alignas(16) glm::vec3 camUpDir;
		alignas(16) f32 verticalFOV;
		u32 numTriangles;
		u32 numSpheres;
		u32 numMaterials;
		u32 numLights;
		u32 maxRayTraceDepth;
		u32 randomState;
	};

	class Raytracer {
		Window window;
		Device device;
		//Renderer renderer{ window, device };

		// createSwapChain
		std::unique_ptr<SwapChain> swapChain;

		// createComputeDescriptorSetLayout
		std::unique_ptr<DescriptorSetLayout> modelToWorldDescriptorSetLayout;
		std::unique_ptr<DescriptorSetLayout> raytraceDescriptorSetLayout;
		std::unique_ptr<DescriptorSetLayout> graphicsDescriptorSetLayout;

		// createComputePipeline
		std::unique_ptr<ComputePipeline> modelToWorldPipeline;
		std::unique_ptr<ComputePipeline> raytracePipeline;
		VkPipelineLayout modelToWorldPipelineLayout;
		VkPipelineLayout raytracePipelineLayout;

		// createComputeImage
		VkImage computeImage;
		VkImageView computeImageView;
		VkDeviceMemory computeImageMemory;
		VkSampler fragmentShaderImageSampler;

		// createGraphicsPipeline
		std::unique_ptr<GraphicsPipeline> graphicsPipeline;
		VkPipelineLayout graphicsPipelineLayout;

		// createShaderStorageBuffers
		std::unique_ptr<RaytraceScene> scene;
		std::unique_ptr<Buffer> scratchBuffer;

		// createUniformBuffers
		std::unique_ptr<Buffer> uniformBuffer;

		// createDesciptorPool
		std::unique_ptr<DescriptorPool> modelToWorldDescriptorPool;
		std::unique_ptr<DescriptorPool> raytraceDescriptorPool;
		std::unique_ptr<DescriptorPool> graphicsDescriptorPool;

		// createComputeDescriptorSets
		std::vector<VkDescriptorSet> modelToWorldDescriptorSets;
		std::vector<VkDescriptorSet> raytraceDescriptorSets;
		std::vector<VkDescriptorSet> graphicsDescriptorSets;

		// createComputeCommandBuffers
		std::vector<VkCommandBuffer> computeCommandBuffers;
		VkCommandBuffer graphicsCommandBuffer;

		VkFence computeComplete;

		// mainLoop -> doIteration
		u32 iteration;
		u32 raysPerPixel;

		std::mt19937 gen{ static_cast<u32>(std::chrono::system_clock::now().time_since_epoch().count()) };
		const f32 scratchSize = 20;

		auto initVulkan() -> void {
			/* Device
				this->createInstance();
				this->setupDebugMessenger();
				this->createSurface();
				this->pickPhysicalDevice();
				this->createLogicalDevice();
				this->createCommandPool();
			*/
			// SwapChain
			this->createSwapChain();
			/*
				this->createImageViews();
				this->createRenderPass();
				this->createFrameBuffers();
			*/
			this->createComputeDescriptorSetLayout();
			this->createGraphicsDescriptorSetLayout();
			this->createComputePipelineLayout();
			this->createComputePipeline();
			this->createComputeImage();
			this->createGraphicsPipeline();

			this->createUniformBuffers();			// in ubo
			this->createScene();					// in ssbo

			this->createComputeDescriptorPool();
			this->createComputeDescriptorSets();
			this->createGraphicsDescriptorPool();
			this->createGraphicsDescriptorSets();

			this->createComputeCommandBuffers();
			this->createGraphicsCommandBuffers();
		}

		auto createSwapChain() -> void;

		auto createComputeDescriptorSetLayout() -> void;
		auto createGraphicsDescriptorSetLayout() -> void;

		auto createComputePipelineLayout() -> void;
		auto createComputePipeline() -> void;

		auto createComputeImage() -> void;

		auto createGraphicsPipeline() -> void;

		auto createScene() -> void;

		auto createUniformBuffers() -> void;

		auto createComputeDescriptorPool() -> void;

		auto createComputeDescriptorSets() -> void;

		auto createGraphicsDescriptorPool() -> void;
		auto createGraphicsDescriptorSets() -> void;

		auto createComputeCommandBuffers() -> void;
		auto createGraphicsCommandBuffers() -> void;

		auto doIteration(f32 frameTime) -> void {
			std::cout << std::format("iteration: {}\n", this->iteration);

			constexpr const auto materialTypeToString = [](SceneTypes::MaterialType mt) -> std::string {
				return mt == SceneTypes::MaterialType::DIFFUSE
					? "Diffuse"
					: (mt == SceneTypes::MaterialType::DIELECTRIC
						? "Dielectric"
						: (mt == SceneTypes::MaterialType::METALLIC
							? "Metallic"
							: (mt == SceneTypes::MaterialType::LIGHT
								? "Light"
								: "N/A"
								)
							)
						);
				};
			if constexpr (Config::ShowBufferDebug) {
				if (this->iteration > 0) {
					std::cout << "after\n";
					auto spheres = this->DEBUGgetDeployedBufferAs<SceneTypes::GPU::Sphere>(
						this->scene->getSphereBuffer()->getBuffer(),
						this->scene->getSphereCount()
					);
					for (auto i = 0; i < this->scene->getSphereCount(); i++) {
						std::cout << std::format("Sphere #{}\n\tcenter: ({}, {}, {}), radius: ({}),\n\tmaterialIndex: ({}), modelIndex: ({})\n",
							i, spheres[i].center.x, spheres[i].center.y, spheres[i].center.z,
							spheres[i].radius, spheres[i].materialIndex, spheres[i].modelIndex
						);
					}
					auto triangles = this->DEBUGgetDeployedBufferAs<SceneTypes::GPU::Triangle>(
						this->scene->getTriangleBuffer()->getBuffer(),
						this->scene->getTriangleCount()
					);
					for (auto i = 0; i < this->scene->getTriangleCount(); i++) {
						std::cout << std::format("Triangle #{}\n\tv0: ({}, {}, {}), v1: ({}, {}, {}), v2: ({}, {}, {}),\n\tmaterialIndex: ({}), modelIndex: ({})\n",
							i, triangles[i].v0.x, triangles[i].v0.y, triangles[i].v0.z,
							triangles[i].v1.x, triangles[i].v1.y, triangles[i].v1.z,
							triangles[i].v2.x, triangles[i].v2.y, triangles[i].v2.z,
							triangles[i].materialIndex, triangles[i].modelIndex
						);
					}
					auto materials = this->DEBUGgetDeployedBufferAs<SceneTypes::GPU::Material>(
						this->scene->getMaterialBuffer()->getBuffer(),
						this->scene->getMaterialCount()
					);
					for (auto i = 0; i < this->scene->getMaterialCount(); i++) {
						std::cout << std::format("Material #{} albedo: ({}, {}, {}), materialType: {} ({})\n",
							i, materials[i].albedo.x, materials[i].albedo.y, materials[i].albedo.z,
							materialTypeToString(materials[i].materialType), (u32)materials[i].materialType
						);
					}
				}
			}

			// fences are for syncing cpu and gpu. semaphores are for specifying the order of gpu tasks
			u32 imageIndex = this->getNextImageIndex(); // await graphics completion
			u32 frameIndex = imageIndex % SwapChain::MAX_FRAMES_IN_FLIGHT;
			// number of frames currently rendering and number of images in swap chain are not the same

			this->scene->updateScene();
			this->scene->getCamera().updateCameraForFrame(this->window, frameTime, this->swapChain->extentAspectRatio());

			if constexpr (Config::ShowBufferDebug) {
				auto resultFromGPU = this->DEBUGgetDeployedBufferAs<SceneTypes::GPU::Model>(
					this->scene->getModelBuffer()->getBuffer(),
					this->scene->getModelCount()
				);
				for (auto i = 0; i < this->scene->getModelCount(); i++) { // first index selects column
					std::cout << std::format("Model #{} matrix: \n[[{}, {}, {}, {}]\n [{}, {}, {}, {}]\n [{}, {}, {}, {}]\n [{}, {}, {}, {}]]\n",
						i, resultFromGPU[i].modelMatrix[0][0], resultFromGPU[i].modelMatrix[1][0], resultFromGPU[i].modelMatrix[2][0], resultFromGPU[i].modelMatrix[3][0],
						resultFromGPU[i].modelMatrix[0][1], resultFromGPU[i].modelMatrix[1][1], resultFromGPU[i].modelMatrix[2][1], resultFromGPU[i].modelMatrix[3][1],
						resultFromGPU[i].modelMatrix[0][2], resultFromGPU[i].modelMatrix[1][2], resultFromGPU[i].modelMatrix[2][2], resultFromGPU[i].modelMatrix[3][2],
						resultFromGPU[i].modelMatrix[0][3], resultFromGPU[i].modelMatrix[1][3], resultFromGPU[i].modelMatrix[2][3], resultFromGPU[i].modelMatrix[3][3]
					);
				}
			}
			if constexpr (Config::ShowBufferDebug) {
				std::cout << "before\n";
				auto resultFromGPU = this->DEBUGgetDeployedBufferAs<SceneTypes::GPU::Sphere>(
					this->scene->getSphereBuffer()->getBuffer(),
					this->scene->getSphereCount()
				);
				for (auto i = 0; i < this->scene->getSphereCount(); i++) {
					std::cout << std::format("Sphere #{} center: ({}, {}, {}), radius: ({}), modelIndex: ({})\n",
						i, resultFromGPU[i].center.x, resultFromGPU[i].center.y, resultFromGPU[i].center.z,
						resultFromGPU[i].radius, resultFromGPU[i].modelIndex
					);
				}
				auto triangles = this->DEBUGgetDeployedBufferAs<SceneTypes::GPU::Triangle>(
					this->scene->getTriangleBuffer()->getBuffer(),
					this->scene->getTriangleCount()
				);
				for (auto i = 0; i < this->scene->getTriangleCount(); i++) {
					std::cout << std::format("Triangle #{}\n\tv0: ({}, {}, {}), v1: ({}, {}, {}), v2: ({}, {}, {}),\n\tmaterialIndex: ({}), modelIndex: ({})\n",
						i, triangles[i].v0.x, triangles[i].v0.y, triangles[i].v0.z,
						triangles[i].v1.x, triangles[i].v1.y, triangles[i].v1.z,
						triangles[i].v2.x, triangles[i].v2.y, triangles[i].v2.z,
						triangles[i].materialIndex, triangles[i].modelIndex
					);
				}
			}
			if constexpr (Config::ShowBufferDebug) {
				auto resultFromGPU = this->DEBUGgetDeployedBufferAs<f32>(
					this->scratchBuffer->getBuffer(),
					this->scratchSize
				);
				for (auto i = 0; i < this->scratchSize; i++) {
					std::cout << std::format("Float #{}: {}\n",
						i, resultFromGPU[i]
					);
				}
			}

			// imageIndex = index of image in swapchain
			// frameIndex = index of frame in flight (ie, set of buffers to use to direct gpu)
			this->recordComputeCommandBuffer(this->computeCommandBuffers[frameIndex], imageIndex);
			this->recordGraphicsCommandBuffer(this->graphicsCommandBuffer, imageIndex);

			RaytracerRenderer::UniformBufferObject nUbo{};
			//nUbo.camPos = glm::vec3(0.0f, 1.0f, 0.0f);
			//nUbo.camDir = glm::vec3(0.0f, -1.0f, 1.0f);
			nUbo.camPos = glm::vec3(275.0f, 275.0f, -800.0f); //glm::vec3(250.0f, 250.0f, -800.0f);
			nUbo.camLookAt = glm::vec3(275.0f, 275.0f, 0.0f);
			nUbo.camUpDir = glm::vec3(0.0, 1.0f, 0.0f);
			nUbo.verticalFOV = this->scene->getCamera().getVerticalFOV();
			nUbo.numTriangles = this->scene->getTriangleCount();
			nUbo.numSpheres = this->scene->getSphereCount();
			nUbo.numMaterials = this->scene->getMaterialCount();
			nUbo.numLights = this->scratchSize;
			nUbo.maxRayTraceDepth = this->scene->getMaxRaytraceDepth();
			nUbo.randomState = this->gen();
			this->uniformBuffer->writeToBuffer(&nUbo);
			this->uniformBuffer->flush(); // make visible to device

			if constexpr (Config::ShowBufferDebug) {
				auto resultFromGPU = this->DEBUGgetDeployedBufferAs<RaytracerRenderer::UniformBufferObject>(
					this->uniformBuffer->getBuffer(),
					1
				);
				std::cout << std::format(
					"UNIFORM BUFFER: camera pos: ({}, {}, {}), lookAt: ({}, {}, {}), \n\tverticalFOV: {}, triangleCount: {}, sphereCount: {}\n\t materialCount: {}, raytraceDepth: {}, randomState: {}\n",
					resultFromGPU[0].camPos.x, resultFromGPU[0].camPos.y, resultFromGPU[0].camPos.z,
					resultFromGPU[0].camLookAt.x, resultFromGPU[0].camLookAt.y, resultFromGPU[0].camLookAt.z,
					resultFromGPU[0].verticalFOV, resultFromGPU[0].numTriangles,
					resultFromGPU[0].numSpheres, resultFromGPU[0].numMaterials,
					resultFromGPU[0].maxRayTraceDepth, resultFromGPU[0].randomState
				);
			}

			if constexpr (Config::Fake1SecondDelay) { // fake 1 second delay
				auto currentTime = std::chrono::high_resolution_clock::now();
				auto newTime = std::chrono::high_resolution_clock::now();
				while (std::chrono::duration<float, std::chrono::milliseconds::period>(newTime - currentTime).count() < 1000.0f)
					newTime = std::chrono::high_resolution_clock::now();
				std::cout << "waited 1 second probably" << std::endl;
			}

			VkSubmitInfo submitInfo = {};
			submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

			VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_ALL_COMMANDS_BIT };
			submitInfo.waitSemaphoreCount = 0;
			submitInfo.pWaitSemaphores = nullptr;
			submitInfo.pWaitDstStageMask = waitStages;

			submitInfo.commandBufferCount = 1;
			submitInfo.pCommandBuffers = &this->computeCommandBuffers[frameIndex];

			vkResetFences(this->device.device(), 1, &this->computeComplete);
			auto submitResult = vkQueueSubmit(this->device.computeQueue(), 1, &submitInfo, this->computeComplete);
			if (submitResult != VK_SUCCESS)
				throw std::runtime_error("failed to submit compute command buffer!");

			auto waitForComputeResult = vkWaitForFences(this->device.device(), 1, &this->computeComplete, VK_TRUE, UINT64_MAX);
			if (waitForComputeResult != VK_SUCCESS)
				throw std::runtime_error("failed to submit draw command buffer!");
			this->swapChain->submitCommandBuffers(&this->graphicsCommandBuffer, &imageIndex);
		}

		auto recordComputeCommandBuffer(VkCommandBuffer, u32) -> void;
		auto recordGraphicsCommandBuffer(VkCommandBuffer, u32) -> void;
		auto beginRenderPass(VkCommandBuffer, u32) -> void;
		auto endRenderPass(VkCommandBuffer) -> void;

		auto getNextImageIndex() -> u32;

		template <typename T, bool Compute = true>
		auto DEBUGgetDeployedBufferAs(VkBuffer, u64) -> std::vector<T>;

	public:
		Raytracer();
		auto mainLoop() -> void {
			this->raysPerPixel = 1;
			auto currentTime = std::chrono::high_resolution_clock::now();

			VkFenceCreateInfo fenceInfo{};
			fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
			fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
			if (vkCreateFence(this->device.device(), &fenceInfo, nullptr, &this->computeComplete) != VK_SUCCESS)
				throw std::runtime_error("failed to create fence");

			while (!this->window.shouldClose()) {
				glfwPollEvents();
				auto newTime = std::chrono::high_resolution_clock::now();
				float frameTime = std::chrono::duration<float, std::chrono::milliseconds::period>(newTime - currentTime).count();
				currentTime = newTime;
				std::cout << "Frame Time: " << frameTime << std::endl;
				doIteration(frameTime);
				this->iteration++;
			}
			vkDeviceWaitIdle(this->device.device());
		}
		~Raytracer();
	};

	template<typename T, bool Compute>
	inline auto Raytracer::Raytracer::DEBUGgetDeployedBufferAs(VkBuffer buffer, u64 elementCount) -> std::vector<T> {
		const auto bufferSize = elementCount * sizeof(T);
		Buffer stagingBuffer(
			device,
			sizeof(T),
			elementCount,
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
		);

		stagingBuffer.map();

		if constexpr (Compute) {
			this->device.copyBuffer(
				this->device.computeQueue(),
				this->device.getComputeCommandPool(),
				buffer,
				stagingBuffer.getBuffer(),
				bufferSize
			);
		}
		else {
			this->device.copyBuffer(
				this->device.graphicsQueue(),
				this->device.getGraphicsCommandPool(),
				buffer,
				stagingBuffer.getBuffer(),
				bufferSize
			);
		}


		T* elems = reinterpret_cast<T*>(stagingBuffer.getMappedMemory());
		stagingBuffer.unmap();

		std::vector<T> resultFromGPU{};
		resultFromGPU.reserve(elementCount);
		for (u32 i = 0; i < elementCount; i++) {
			resultFromGPU.push_back(elems[i]);
		}
		return resultFromGPU;
	}
};

