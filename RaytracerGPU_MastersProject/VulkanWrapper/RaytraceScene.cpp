
#include "RaytraceScene.hpp"

#include "utils.hpp"
#include "../utils/Functions.hpp"
#include "../utils/Functors.hpp"

#include <variant>
#include <memory>
#include <stdexcept>

RaytraceScene::RaytraceScene(Device& device) :
	device(device), camera{CameraGameObject::makeCameraGameObject()}
{}

RaytraceScene::~RaytraceScene() {}

auto RaytraceScene::bind(VkCommandBuffer commandBuffer) -> void {
	
}

auto RaytraceScene::draw(VkCommandBuffer commandBuffer) -> void {

}

auto RaytraceScene::setMaxRaytraceDepth(u32 depth) -> void {
	this->maxRaytraceDepth = depth;
}

auto RaytraceScene::setRaysPerPixel(u32 raysPerPixel) -> void {
	this->raysPerPixel = raysPerPixel;
}

auto RaytraceScene::getMaxRaytraceDepth() -> u32 {
	return this->maxRaytraceDepth;
}

auto RaytraceScene::getRaysPerPixel() -> u32 {
	return this->raysPerPixel;
}

auto RaytraceScene::getCamera() -> CameraGameObject& {
	return this->camera;
}

auto RaytraceScene::addGameObject(GameObject&& gameObject) -> void {
	if (this->buffersCreated) {
		throw std::runtime_error("temp error: gameobject added after buffers deployed");
	}
	if (gameObject.getModel() != nullptr)
		this->gameObjects.push_back(gameObject);
}

auto RaytraceScene::getGameObject(GameObjectId id) -> GameObject& {
	for (auto& gameObject : this->gameObjects) {
		if (id == gameObject.getId())
			return gameObject;
	}
}

//auto RaytraceScene::getGameObject(size_t index) -> GameObject& {
//	return this->gameObjects.at(index);
//}

auto RaytraceScene::removeGameObject(GameObjectId id) -> bool {
	return false; // TODO
}

auto RaytraceScene::removeGameObject(size_t index) -> bool {
	return false; // TODO
}

auto RaytraceScene::prepForRender() -> void {
	this->moveGameObjectsToHostVectors();
	this->createBuffers();
}

auto RaytraceScene::updateScene() -> void {
	if (this->buffersCreated) {
		this->moveGameObjectsToHostVectors();
		if (this->models.size() > this->modelCountMax) {
			this->createModelBuffer();
			std::runtime_error("Cannot update yet. Need to fix descriptor sets when deallocating buffers and making new ones.");
		}
		else {
			this->updateModelBuffer();
		}
		if (this->triangles.size() > this->triangleCountMax) {
			this->createTriangleBuffer();
			std::runtime_error("Cannot update yet. Need to fix descriptor sets when deallocating buffers and making new ones.");
		}
		else {
			this->updateTriangleBuffer();
		}
		if (this->spheres.size() > this->sphereCountMax) {
			this->createSphereBuffer();
			std::runtime_error("Cannot update yet. Need to fix descriptor sets when deallocating buffers and making new ones.");
		}
		else {
			this->updateSphereBuffer();
		}
		if (this->materials.size() > this->materialCountMax) {
			this->createMaterialBuffer();
			std::runtime_error("Cannot update yet. Need to fix descriptor sets when deallocating buffers and making new ones.");
		}
		else {
			this->updateMaterialBuffer();
		}
	}
	else {
		this->prepForRender();
	}
}

auto RaytraceScene::getModelBuffer() -> std::unique_ptr<Buffer>& {
	return this->modelBuffer;
}

auto RaytraceScene::getTriangleBuffer() -> std::unique_ptr<Buffer>& {
	return this->triangleBuffer;
}

auto RaytraceScene::getSphereBuffer() -> std::unique_ptr<Buffer>& {
	return this->sphereBuffer;
}

auto RaytraceScene::getMaterialBuffer() -> std::unique_ptr<Buffer>& {
	return this->materialBuffer;
}

auto RaytraceScene::getModelCount() -> u32 {
	return this->modelCount;
}

auto RaytraceScene::getTriangleCount() -> u32 {
	return this->triangleCount;
}

auto RaytraceScene::getSphereCount() -> u32 {
	return this->sphereCount;
}

auto RaytraceScene::getMaterialCount() -> u32 {
	return this->materialCount;
}

auto RaytraceScene::createBuffers() -> void {
	this->moveGameObjectsToHostVectors();
	this->createModelBuffer();
	this->createTriangleBuffer();
	this->createSphereBuffer();
	this->createMaterialBuffer();
	//this->createLightBuffer(this->lights);
	this->buffersCreated = true;
}

auto RaytraceScene::moveGameObjectsToHostVectors() -> void {
	this->models.clear();
	this->triangles.clear();
	this->spheres.clear();
	this->materials.clear();
	//this->lights.clear();
	for (auto i = 0; i < this->gameObjects.size(); i++) {
		const auto modelIndex = this->models.size();
		this->models.emplace_back(this->gameObjects[i].transform.mat4());
		if (
			auto triangles = getVariantFromSharedPtr<RTModel_Triangles>(this->gameObjects[i].getModel());
			triangles != nullptr
		) {
			const auto materialIndex = this->materials.size();
			this->materials.push_back(triangles->getMaterialType());
			const auto& triangleList = triangles->getTriangles();
			for (const auto& triangle : triangleList) {
				this->triangles.push_back(
					SceneTypes::GPU::Triangle::convertFromCPUTriangle(triangle, materialIndex, modelIndex)
				);
			}
		}
		else if (
			auto sphere = getVariantFromSharedPtr<RTModel_Sphere>(this->gameObjects[i].getModel());
			sphere != nullptr
		) {
			const auto materialIndex = this->materials.size();
			this->materials.push_back(sphere->getMaterialType());
			this->spheres.push_back(
				SceneTypes::GPU::Sphere{
					sphere->getCenter(),
					sphere->getRadius(),
					static_cast<u32>(materialIndex),
					static_cast<u32>(modelIndex)
				}
			);
		}
	}
	this->modelCount = static_cast<u32>(glm::max<size_t>(this->models.size(), 1));
	this->triangleCount = static_cast<u32>(glm::max<size_t>(this->triangles.size(), 1));
	this->sphereCount = static_cast<u32>(glm::max<size_t>(this->spheres.size(), 1));
	this->materialCount = static_cast<u32>(glm::max<size_t>(this->materials.size(), 1));
	// need min 1 to allocate. if 1 is allocated and none present, works fine, just ignores extra allocated space till used
}

auto RaytraceScene::createModelBuffer() -> void {
	RaytraceScene::constructStagingAndDeviceBufferAndCopyToDevice(
		this->device,
		this->models,
		this->modelBuffer,
		this->device.computeQueue(),
		this->device.getComputeCommandPool(),
		this->modelCount
	);
	this->modelCountMax = this->modelCount;
}

auto RaytraceScene::createTriangleBuffer() -> void {
	RaytraceScene::constructStagingAndDeviceBufferAndCopyToDevice(
		this->device,
		this->triangles,
		this->triangleBuffer,
		this->device.computeQueue(),
		this->device.getComputeCommandPool(),
		this->triangleCount
	);
	this->triangleCountMax = this->triangleCount;
}

auto RaytraceScene::createSphereBuffer() -> void {
	RaytraceScene::constructStagingAndDeviceBufferAndCopyToDevice(
		this->device,
		this->spheres,
		this->sphereBuffer,
		this->device.computeQueue(),
		this->device.getComputeCommandPool(),
		this->sphereCount
	);
	this->sphereCountMax = this->sphereCount;
}

auto RaytraceScene::createMaterialBuffer() -> void {
	RaytraceScene::constructStagingAndDeviceBufferAndCopyToDevice(
		this->device,
		this->materials,
		this->materialBuffer,
		this->device.computeQueue(),
		this->device.getComputeCommandPool(),
		this->materialCount
	);
	this->materialCountMax = this->materialCount;
}

//auto RaytraceScene::createLightBuffer(const std::vector<SceneTypes::GPU::Light>& lights) -> void {
//	RaytraceScene::constructStagingAndDeviceBufferAndCopyToDevice(
//		this->device,
//		lights,
//		this->lightBuffer,
//		this->device.computeQueue(),
//		this->device.getComputeCommandPool(),
//		this->lightCount
//	);
//}

auto RaytraceScene::updateModelBuffer() -> void {
	RaytraceScene::constructStagingBufferAndCopyToDevice(
		this->device,
		this->models,
		this->modelBuffer,
		this->device.computeQueue(),
		this->device.getComputeCommandPool(),
		this->modelCount
	);
}

auto RaytraceScene::updateTriangleBuffer() -> void {
	RaytraceScene::constructStagingBufferAndCopyToDevice(
		this->device,
		this->triangles,
		this->triangleBuffer,
		this->device.computeQueue(),
		this->device.getComputeCommandPool(),
		this->triangleCount
	);
}

auto RaytraceScene::updateSphereBuffer() -> void {
	RaytraceScene::constructStagingBufferAndCopyToDevice(
		this->device,
		this->spheres,
		this->sphereBuffer,
		this->device.computeQueue(),
		this->device.getComputeCommandPool(),
		this->sphereCount
	);
}

auto RaytraceScene::updateMaterialBuffer() -> void {
	RaytraceScene::constructStagingBufferAndCopyToDevice(
		this->device,
		this->materials,
		this->materialBuffer,
		this->device.computeQueue(),
		this->device.getComputeCommandPool(),
		this->materialCount
	);
}
