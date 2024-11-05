#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS					// functions expect radians, not degrees
#define GLM_FORCE_DEPTH_ZERO_TO_ONE			// Depth buffer values will range from 0 to 1, not -1 to 1
#include <glm/glm.hpp>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>

#include "../utils/PrimitiveTypes.hpp"

#include "SceneTypes.hpp"
#include "Device.hpp"
#include "Buffer.hpp"
#include "utils.hpp"
#include "GameObject.hpp"
#include "CameraGameObject.hpp"

#include <cassert>
#include <vector>
#include <memory>
#include <iostream>
#include <unordered_map>

class RaytraceScene {
	Device& device;

	CameraGameObject camera; // scene camera

	std::vector<GameObject> gameObjects; // gameobjects (any in scene including those yet to be added to buffers for rendering)
	bool redeployBuffers; // set to true if a game object is added or removed, false otherwise, set false after redeploy

	// temp containers for model contents on cpu side. on game object add or remove, can reuse to avoid redeploying all other game objects
	std::vector<SceneTypes::GPU::Model> models;
	std::vector<SceneTypes::GPU::Triangle> triangles;
	std::vector<SceneTypes::GPU::Sphere> spheres;
	std::vector<SceneTypes::GPU::Material> materials;
	std::vector<SceneTypes::GPU::Light> lights;

	// buffer objects on gpu and associated element counts
	std::unique_ptr<Buffer> modelBuffer;
	u32 modelCount;
	u32 modelCountMax;
	std::unique_ptr<Buffer> triangleBuffer;
	u32 triangleCount;
	u32 triangleCountMax;
	std::unique_ptr<Buffer> sphereBuffer;
	u32 sphereCount;
	u32 sphereCountMax;
	std::unique_ptr<Buffer> materialBuffer;
	u32 materialCount;
	u32 materialCountMax;
	//std::unique_ptr<Buffer> lightBuffer;
	//u32 lightCount;

	u32 maxRaytraceDepth;
	u32 raysPerPixel;
	// some methods throw runtime errors if called before buffers are set
	bool buffersCreated;

public:
	RaytraceScene(Device& device);
	~RaytraceScene();

	RaytraceScene(const RaytraceScene&) = delete;
	RaytraceScene& operator=(const RaytraceScene&) = delete;

	auto bind(VkCommandBuffer commandBuffer) -> void;
	auto draw(VkCommandBuffer commandBuffer) -> void;

	auto setMaxRaytraceDepth(u32) -> void;
	auto setRaysPerPixel(u32) -> void;
	auto getMaxRaytraceDepth() -> u32;
	auto getRaysPerPixel() -> u32;

	auto getCamera() -> CameraGameObject&;

	auto addGameObject(GameObject&& gameObject) -> void;

	auto getGameObject(GameObjectId id) -> GameObject&;
	auto getGameObject(size_t index) -> GameObject&;

	auto removeGameObject(GameObjectId id) -> bool;
	auto removeGameObject(size_t index) -> bool;

	auto prepForRender() -> void;
	auto updateScene() -> void;

	auto getModelBuffer() -> std::unique_ptr<Buffer>&;
	auto getTriangleBuffer() -> std::unique_ptr<Buffer>&;
	auto getSphereBuffer() -> std::unique_ptr<Buffer>&;
	auto getMaterialBuffer() -> std::unique_ptr<Buffer>&;
	//auto getLightBuffer() -> std::unique_ptr<Buffer>&;

	auto getModelCount() -> u32;
	auto getTriangleCount() -> u32;
	auto getSphereCount() -> u32;
	auto getMaterialCount() -> u32;
	//auto getLightCount() -> u32;

	auto findEnclosingAABB(const std::vector<SceneTypes::GPU::AABB>&) -> std::pair<glm::vec3, glm::vec3>;

private:
	auto createBuffers() -> void;
	auto moveGameObjectsToHostVectors() -> void;
	auto createModelBuffer() -> void;
	auto createTriangleBuffer() -> void;
	auto createSphereBuffer() -> void;
	auto createMaterialBuffer() -> void;
	//auto createLightBuffer(const std::vector<SceneTypes::GPU::Light>& lights) -> void;

	auto updateModelBuffer() -> void;
	auto updateTriangleBuffer() -> void;
	auto updateSphereBuffer() -> void;
	auto updateMaterialBuffer() -> void;

	template <typename T>
	static auto constructStagingAndDeviceBufferAndCopyToDevice(
		Device&,
		std::vector<T>,
		std::unique_ptr<Buffer>&,
		VkQueue,
		VkCommandPool,
		u32&
	) -> void;

	template <typename T>
	static auto constructStagingBufferAndCopyToDevice(
		Device&,
		std::vector<T>,
		std::unique_ptr<Buffer>&,
		VkQueue,
		VkCommandPool,
		u32&
	) -> void;
};

template<typename T>
inline auto RaytraceScene::constructStagingAndDeviceBufferAndCopyToDevice(
	Device& device,
	std::vector<T> elementsToTransfer,
	std::unique_ptr<Buffer>& bufferToTransferTo,
	VkQueue destinationQueue,
	VkCommandPool destinationCommandPool,
	u32& newSize
) -> void {
	constexpr const VkDeviceSize elementSize = sizeof(T);
	const auto len = elementsToTransfer.size();
	newSize = static_cast<u32>(len);

	Buffer stagingBuffer(
		device,
		elementSize,
		len,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT, // will transfer from
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
	);

	stagingBuffer.map();
	stagingBuffer.writeToBuffer((void*) elementsToTransfer.data());

	bufferToTransferTo = std::make_unique<Buffer>( // setup gpu buffer
		device,
		elementSize,
		len,
		VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT, // is ssbo and will transfer into
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT // TEMP: VK_BUFFER_USAGE_TRANSFER_SRC_BIT is here for debug purposes and should be removed
	);

	device.copyBuffer( // copy staging buffer into gpu buffer
		destinationQueue,
		destinationCommandPool,
		stagingBuffer.getBuffer(),
		bufferToTransferTo->getBuffer(),
		elementSize * len
	);
}

template<typename T>
inline auto RaytraceScene::constructStagingBufferAndCopyToDevice(
	Device& device,
	std::vector<T> elementsToTransfer,
	std::unique_ptr<Buffer>& bufferToTransferTo,
	VkQueue destinationQueue,
	VkCommandPool destinationCommandPool,
	u32& newSize
) -> void {
	constexpr const VkDeviceSize elementSize = sizeof(T);
	const auto len = elementsToTransfer.size();
	newSize = static_cast<u32>(len);

	Buffer stagingBuffer(
		device,
		elementSize,
		len,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT, // will transfer from
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
	);

	stagingBuffer.map();
	stagingBuffer.writeToBuffer((void*)elementsToTransfer.data());

	/*bufferToTransferTo = std::make_unique<Buffer>( // setup gpu buffer
		device,
		elementSize,
		len,
		VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, // is ssbo and will transfer into
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
	);*/

	device.copyBuffer( // copy staging buffer into gpu buffer
		destinationQueue,
		destinationCommandPool,
		stagingBuffer.getBuffer(),
		bufferToTransferTo->getBuffer(),
		elementSize * len
	);
}
