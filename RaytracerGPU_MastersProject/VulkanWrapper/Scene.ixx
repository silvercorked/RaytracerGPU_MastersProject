module;

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS					// functions expect radians, not degrees
#define GLM_FORCE_DEPTH_ZERO_TO_ONE			// Depth buffer values will range from 0 to 1, not -1 to 1
#include <glm/glm.hpp>

#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>

#include <cassert>

export module VulkanWrap:Scene;

import :Device;
import :Buffer;
import :Utils;
import PrimitiveTypes;

import <vector>;
import <memory>;
import <iostream>;
import <unordered_map>;

export class Scene {
	Device& device;

	std::unique_ptr<Buffer> triangleBuffer;
	u32 triangleCount;

	std::unique_ptr<Buffer> sphereBuffer;
	u32 sphereCount;

	std::unique_ptr<Buffer> lightBuffer;
	u32 lightCount;

	std::unique_ptr<Buffer> materialBuffer;
	u32 materialCount;

public:
	struct Triangle {
		glm::vec3 position;
	};
}