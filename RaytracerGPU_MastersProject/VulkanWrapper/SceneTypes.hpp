#pragma once

#include <vulkan/vulkan.h>

#define GLM_FORCE_RADIANS					// functions expect radians, not degrees
#define GLM_FORCE_DEPTH_ZERO_TO_ONE			// Depth buffer values will range from 0 to 1, not -1 to 1
#include <glm/glm.hpp>

#include "../utils/PrimitiveTypes.hpp"

#include <vector>

namespace SceneTypes {
	enum class MaterialType : u32 {
		LIGHT = 0,
		DIFFUSE = 1,
		METALLIC = 2,
		DIELECTRIC = 3
	};
	namespace CPU {
		struct Triangle {
			glm::vec3 v0;
			glm::vec3 v1;
			glm::vec3 v2;
		};
		struct Sphere {
			glm::vec3 center;
			f32 radius;
		};
	}
	namespace GPU { // meant to be stored in SSBOs and have strict sizes and alignments for use on gpu
		struct Model {
			glm::mat4 modelMatrix;

			constexpr auto getSize() const -> const VkDeviceSize { return sizeof(SceneTypes::GPU::Model); }

			bool operator==(const Model& other) const {
				return this->modelMatrix == other.modelMatrix;
			}
		};
		struct Triangle {
			alignas(16) glm::vec3 v0;
			alignas(16) glm::vec3 v1;
			alignas(16) glm::vec3 v2;
			alignas(16) u32 materialIndex;
			u32 modelIndex;

			constexpr auto getSize() const -> const VkDeviceSize { return sizeof(SceneTypes::GPU::Triangle); }

			static auto convertFromCPUTriangle(const SceneTypes::CPU::Triangle& tri, u32 matIndex, u32 modIndex) -> SceneTypes::GPU::Triangle {
				return { tri.v0, tri.v1, tri.v2, matIndex, modIndex };
			}

			bool operator==(const Triangle& other) const {
				return this->v0 == other.v0
					&& this->v1 == other.v1
					&& this->v2 == other.v2
					&& this->materialIndex == other.materialIndex
					&& this->modelIndex == other.modelIndex;
			}
		};
		struct Sphere { // watch members for alignment
			alignas(16) glm::vec3 center;
			alignas(16) f32 radius;
			u32 materialIndex;
			u32 modelIndex;

			constexpr auto getSize() const -> const VkDeviceSize { return sizeof(SceneTypes::GPU::Sphere); }

			static auto convertFromCPUSphere(const SceneTypes::CPU::Sphere& sp, u32 matIndex, u32 modIndex) -> SceneTypes::GPU::Sphere {
				return { sp.center, sp.radius, matIndex, modIndex};
			}

			bool operator==(const Sphere& other) const {
				return this->center == other.center
					&& this->radius == other.radius
					&& this->materialIndex == other.materialIndex
					&& this->materialIndex == other.modelIndex
					&& this->modelIndex == other.modelIndex;
			}
		};
		struct Material { // watch members for alignment
			alignas(16) glm::vec3 albedo;
			alignas(8) SceneTypes::MaterialType materialType;

			Material() : albedo{ 0 }, materialType{ SceneTypes::MaterialType::DIFFUSE } {}
			Material(glm::vec3 color, SceneTypes::MaterialType type) : albedo{ color }, materialType{ type } {}

			constexpr auto getSize() const -> const VkDeviceSize { return sizeof(SceneTypes::GPU::Material); }

			bool operator==(const Material& other) const {
				return this->albedo == other.albedo
					&& this->materialType == other.materialType;
			}
		};
		struct Light {
			f32 area;
			u32 triangleIndex;

			constexpr auto getSize() const -> const VkDeviceSize { return sizeof(SceneTypes::GPU::Light); }

			bool operator==(const Light& other) const {
				return this->area == other.area
					&& this->triangleIndex == other.triangleIndex;
			}
		};
		struct AABB {
			alignas(16) glm::vec3 center;
			alignas(16) f32 minX; f32 maxX;
			f32 minY; f32 maxY;
			f32 minZ; f32 maxZ;
			u32 index;
			u32 primitiveType; // 0 -> sphere, 1 -> triangle
		};
		struct MortonPrimitive {
			u32 code;
			u32 aabbIndex;
		};
		struct BVHNode {
			AABB aabb;
			u32 left;
			u32 right;
			u32 aabbIndex;
		};
	};

	using Material = SceneTypes::GPU::Material;
};
