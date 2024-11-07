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
#include <bitset>
#include "VulkanWrapper/SceneTypes.hpp"

namespace RaytracerBVHRenderer {
	struct RaytracingUniformBufferObject {
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
	struct EnclosingAABBUniformBufferObject {
		alignas(16) glm::vec3 min;
		alignas(16) glm::vec3 max;
	};
	struct FragmentUniformBufferObject {
		u32 raysPerPixel; // used in gamma correction
	};

	class Raytracer {
		Window window;
		Device device;
		//Renderer renderer{ window, device };

		// createSwapChain
		std::unique_ptr<SwapChain> swapChain;

		// createComputeDescriptorSetLayout
		std::unique_ptr<DescriptorSetLayout> modelToWorldDescriptorSetLayout;
		std::unique_ptr<DescriptorSetLayout> constructAABBDescriptorSetLayout;
		std::unique_ptr<DescriptorSetLayout> generateMortonCodeDescriptorSetLayout;
		std::unique_ptr<DescriptorSetLayout> radixSortDescriptorSetLayout;
		//std::unique_ptr<DescriptorSetLayout> raytraceDescriptorSetLayout;
		//std::unique_ptr<DescriptorSetLayout> graphicsDescriptorSetLayout;

		// createComputePipeline
		std::unique_ptr<ComputePipeline> modelToWorldPipeline;
		std::unique_ptr<ComputePipeline> constructAABBPipeline;
		std::unique_ptr<ComputePipeline> generateMortonCodePipeline;
		std::unique_ptr<ComputePipeline> radixSortComputePipeline;
		//std::unique_ptr<ComputePipeline> raytracePipeline;
		VkPipelineLayout modelToWorldPipelineLayout;
		VkPipelineLayout constructAABBPipelineLayout;
		VkPipelineLayout generateMortonCodePipelineLayout;
		VkPipelineLayout radixSortPipelineLayout;
		//VkPipelineLayout raytracePipelineLayout;

		// createComputeImage
		//VkImage computeImage;
		//VkImageView computeImageView;
		//VkDeviceMemory computeImageMemory;
		//VkSampler fragmentShaderImageSampler;

		// createGraphicsPipeline
		//std::unique_ptr<GraphicsPipeline> graphicsPipeline;
		//VkPipelineLayout graphicsPipelineLayout;

		// createShaderStorageBuffers
		std::unique_ptr<RaytraceScene> scene;
		std::unique_ptr<Buffer> scratchBuffer;
		// temp buffers for debugging
		std::unique_ptr<Buffer> AABBBuffer;
		std::unique_ptr<Buffer> mortonPrimitiveBuffer1;
		std::unique_ptr<Buffer> mortonPrimitiveBuffer2;

		// createUniformBuffers
		std::unique_ptr<Buffer> rayUniformBuffer;
		std::unique_ptr<Buffer> enclosingAABBUniformBuffer;
		//std::unique_ptr<Buffer> fragUniformBuffer;

		// createDesciptorPool
		std::unique_ptr<DescriptorPool> modelToWorldDescriptorPool;
		std::unique_ptr<DescriptorPool> constructAABBDescriptorPool;
		std::unique_ptr<DescriptorPool> generateMortonCodeDescriptorPool;
		std::unique_ptr<DescriptorPool> radixSortDescriptorPool;
		//std::unique_ptr<DescriptorPool> raytraceDescriptorPool;
		//std::unique_ptr<DescriptorPool> graphicsDescriptorPool;

		// createComputeDescriptorSets
		std::vector<VkDescriptorSet> modelToWorldDescriptorSets;
		std::vector<VkDescriptorSet> constructAABBDescriptorSets;
		std::vector<VkDescriptorSet> generateMortonCodeDescriptorSets;
		std::vector<VkDescriptorSet> radixSortDescriptorSets;
		//std::vector<VkDescriptorSet> raytraceDescriptorSets;
		//std::vector<VkDescriptorSet> graphicsDescriptorSets;

		// createComputeCommandBuffers
		std::vector<VkCommandBuffer> computeS1CommandBuffers;
		std::vector<VkCommandBuffer> computeS2CommandBuffers;
		VkCommandBuffer graphicsCommandBuffer;

		VkFence computeS1Complete;
		VkFence computeS2Complete;

		// mainLoop -> doIteration
		u32 iteration;

		std::mt19937 gen{ static_cast<u32>(std::chrono::system_clock::now().time_since_epoch().count()) };
		const f32 scratchSize = 20;

		std::vector<std::vector<std::chrono::microseconds>> times;

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
			u32 imageIndex = 0; //this->getNextImageIndex(); // await graphics completion
			u32 frameIndex = 0; //imageIndex% SwapChain::MAX_FRAMES_IN_FLIGHT;
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
			this->recordComputeS1CommandBuffer(this->computeS1CommandBuffers[frameIndex], imageIndex);
			this->recordComputeS2CommandBuffer(this->computeS2CommandBuffers[frameIndex], imageIndex);
			//this->recordGraphicsCommandBuffer(this->graphicsCommandBuffer, imageIndex);

			RaytracerBVHRenderer::RaytracingUniformBufferObject rUbo{};
			rUbo.camPos = glm::vec3(275.0f, 275.0f, -800.0f);
			rUbo.camLookAt = glm::vec3(275.0f, 275.0f, 0.0f);
			rUbo.camUpDir = glm::vec3(0.0, 1.0f, 0.0f);
			rUbo.verticalFOV = this->scene->getCamera().getVerticalFOV();
			rUbo.numTriangles = this->scene->getTriangleCount();
			rUbo.numSpheres = this->scene->getSphereCount();
			rUbo.numMaterials = this->scene->getMaterialCount();
			rUbo.numLights = this->scratchSize;
			rUbo.maxRayTraceDepth = this->scene->getMaxRaytraceDepth();
			rUbo.randomState = this->gen();
			this->rayUniformBuffer->writeToBuffer(&rUbo);
			this->rayUniformBuffer->flush(); // make visible to device

			/*
			RaytracerBVHRenderer::FragmentUniformBufferObject fUbo{};
			fUbo.raysPerPixel = this->scene->getRaysPerPixel();
			this->fragUniformBuffer->writeToBuffer(&fUbo);
			this->fragUniformBuffer->flush();
			*/

			if constexpr (Config::ShowBufferDebug) {
				auto resultFromGPU = this->DEBUGgetDeployedBufferAs<RaytracerBVHRenderer::RaytracingUniformBufferObject>(
					this->rayUniformBuffer->getBuffer(),
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

			VkSubmitInfo submitInfoS1 = {};
			submitInfoS1.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

			VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_ALL_COMMANDS_BIT };
			submitInfoS1.waitSemaphoreCount = 0;
			submitInfoS1.pWaitSemaphores = nullptr;
			submitInfoS1.pWaitDstStageMask = waitStages;

			submitInfoS1.commandBufferCount = 1;
			submitInfoS1.pCommandBuffers = &this->computeS1CommandBuffers[frameIndex];

			vkResetFences(this->device.device(), 1, &this->computeS1Complete);
			auto subRes1 = vkQueueSubmit(this->device.computeQueue(), 1, &submitInfoS1, this->computeS1Complete);
			if (subRes1 != VK_SUCCESS)
				throw std::runtime_error("failed to submit compute command buffer!");

			auto waitForComputeResult1 = vkWaitForFences(this->device.device(), 1, &this->computeS1Complete, VK_TRUE, UINT64_MAX);
			if (waitForComputeResult1 != VK_SUCCESS)
				throw std::runtime_error("failed to submit draw command buffer!");
			
			auto AABBs = this->DEBUGgetDeployedBufferAs<SceneTypes::GPU::AABB>(
				this->AABBBuffer->getBuffer(),
				this->scene->getTriangleCount() + this->scene->getSphereCount()
			);
			auto [minVec3, maxVec3] = this->scene->findEnclosingAABB(AABBs);
			RaytracerBVHRenderer::EnclosingAABBUniformBufferObject enclosing = {minVec3, maxVec3};
			this->enclosingAABBUniformBuffer->writeToBuffer(&enclosing);
			this->enclosingAABBUniformBuffer->flush();

			VkSubmitInfo submitInfoS2 = {};
			submitInfoS2.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
			submitInfoS2.waitSemaphoreCount = 0;
			submitInfoS2.pWaitSemaphores = nullptr;
			submitInfoS2.pWaitDstStageMask = waitStages;
			submitInfoS2.commandBufferCount = 1;
			submitInfoS2.pCommandBuffers = &this->computeS2CommandBuffers[frameIndex];

			vkResetFences(this->device.device(), 1, &this->computeS2Complete);
			auto subRes2 = vkQueueSubmit(this->device.computeQueue(), 1, &submitInfoS2, this->computeS2Complete);

			auto waitForComputeResult2 = vkWaitForFences(this->device.device(), 1, &this->computeS2Complete, VK_TRUE, UINT64_MAX);
			if (waitForComputeResult2 != VK_SUCCESS)
				throw std::runtime_error("failed to submit draw command buffer!");
			
			//

			auto mortonPrimitives = this->DEBUGgetDeployedBufferAs<SceneTypes::GPU::MortonPrimitive>(
				this->mortonPrimitiveBuffer2->getBuffer(),
				this->scene->getTriangleCount() + this->scene->getSphereCount()
			);

			std::cout << std::format("Enclosing AABB: min({}, {}, {}), max({}, {}, {})",
				minVec3.x, minVec3.y, minVec3.z,
				maxVec3.x, maxVec3.y, maxVec3.z
			);

			std::cout << "morton Primitives:\n";
			for (auto i = 0; i < mortonPrimitives.size(); i++) {
				SceneTypes::GPU::AABB aabb = AABBs[mortonPrimitives[i].aabbIndex];
				std::cout << std::format(
					"i: {}, aabbCenter({}, {}, {})\n\taabbMin({}, {}, {}), aabbMax({}, {}, {})\n\taabbIndex: {}, mortPrim: 0b{:032b}\n",
					i, aabb.center.x, aabb.center.y, aabb.center.z, aabb.minX, aabb.minY, aabb.minZ, aabb.maxX, aabb.maxY, aabb.maxZ,
					mortonPrimitives[i].aabbIndex, mortonPrimitives[i].code
				);
			}

			//

			vkResetFences(this->device.device(), 1, &this->computeS2Complete);

			//this->swapChain->submitCommandBuffers(&this->graphicsCommandBuffer, &imageIndex);
		}

		auto recordComputeS1CommandBuffer(VkCommandBuffer, u32) -> void;
		auto recordComputeS2CommandBuffer(VkCommandBuffer, u32) -> void;
		auto recordGraphicsCommandBuffer(VkCommandBuffer, u32) -> void;
		auto beginRenderPass(VkCommandBuffer, u32) -> void;
		auto endRenderPass(VkCommandBuffer) -> void;

		auto getNextImageIndex() -> u32;

		template <typename T, bool Compute = true>
		auto DEBUGgetDeployedBufferAs(VkBuffer, u64) -> std::vector<T>;

	public:
		Raytracer();
		auto mainLoop() -> void {
			auto currentTime = std::chrono::high_resolution_clock::now();

			VkFenceCreateInfo fenceInfo{};
			fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
			fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
			if (vkCreateFence(this->device.device(), &fenceInfo, nullptr, &this->computeS1Complete) != VK_SUCCESS)
				throw std::runtime_error("failed to create fence");
			if (vkCreateFence(this->device.device(), &fenceInfo, nullptr, &this->computeS2Complete) != VK_SUCCESS)
				throw std::runtime_error("failed to create fence");

			if constexpr (Config::RunRayPerPixelIncreasingDemo) {
				this->scene->setRaysPerPixel(Config::RayPerPixelIncreasingDemoConfig::startRaysPerPixel);
			}

			while (!this->window.shouldClose()) {
				glfwPollEvents();
				auto newTime = std::chrono::high_resolution_clock::now();
				auto frameTime = std::chrono::duration_cast<std::chrono::microseconds>(newTime - currentTime);
				currentTime = newTime;
				std::cout << "Frame Time(us): " << frameTime
					<< " RaysPerPixel: " << this->scene->getRaysPerPixel()
					<< " Depth: " << this->scene->getMaxRaytraceDepth()
					<< std::endl;
				doIteration(std::chrono::duration<float, std::chrono::microseconds::period>(frameTime).count());

				if constexpr (Config::RunRayPerPixelIncreasingDemo) {
					if (this->iteration != 0) {
						u32 index = (this->iteration - 1) / Config::RayPerPixelIncreasingDemoConfig::runsBeforeIncrease;
						if (this->scene->getRaysPerPixel() > Config::RayPerPixelIncreasingDemoConfig::maxRaysPerPixel) {
							break;
						}
						if (this->times.size() == index) {
							this->times.push_back(std::vector<std::chrono::microseconds>{frameTime});
						}
						else {
							this->times[index].push_back(frameTime);
						}
						if (this->iteration % Config::RayPerPixelIncreasingDemoConfig::runsBeforeIncrease == 0) {
							this->scene->setRaysPerPixel(this->scene->getRaysPerPixel() + Config::RayPerPixelIncreasingDemoConfig::increaseAmount);
						}
					}
				}

				this->iteration++;
			}
			vkDeviceWaitIdle(this->device.device());

			if constexpr (Config::RunRayPerPixelIncreasingDemo) {
				std::ofstream out;
				out.open("runtimes.csv", std::ios::out | std::ios::trunc);
				for (auto i = 0; i < this->times.size(); i++) {
					std::chrono::microseconds sum = this->times[i].at(0); // assumes at least 1
					for (auto j = 1; j < Config::RayPerPixelIncreasingDemoConfig::runsBeforeIncrease; j++) {
						sum += this->times[i][j];
					}
					out << (i + 1) << ", " << sum / Config::RayPerPixelIncreasingDemoConfig::runsBeforeIncrease << ",\n";
				}
				out.close();
			}
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

