#pragma once

#define GLM_FORCE_RADIANS					// functions expect radians, not degrees
#define GLM_FORCE_DEPTH_ZERO_TO_ONE			// Depth buffer values will range from 0 to 1, not -1 to 1
#include <glm/glm.hpp>

#include "../utils/PrimitiveTypes.hpp"

#include "SceneTypes.hpp"
#include "utils.hpp"

#include <vector>
#include <string>
#include <variant>
#include <memory>

struct Vertex {
	glm::vec3 position{};
	glm::vec3 color{};
	glm::vec3 normal{};
	glm::vec2 uv{};
	
	bool operator==(const Vertex& other) const {
		return this->position == other.position
			&& this->color == other.color
			&& this->normal == other.normal
			&& this->uv == other.uv;
	}
};

template<>
struct std::hash<Vertex> {
	size_t operator()(const Vertex& vertex) const {
		size_t seed = 0;
		hashCombine(seed, vertex.position, vertex.color, vertex.normal, vertex.uv);
		return seed;
	}
};
template <>
struct std::hash<glm::vec3> {
	size_t operator()(const glm::vec3& v) const {
		size_t seed = 0;
		hashCombine(seed, v.x, v.y, v.z);
		return seed;
	}
};
template <>
struct std::hash<glm::vec2> {
	size_t operator()(const glm::vec2& v) const {
		size_t seed = 0;
		hashCombine(seed, v.x, v.y);
		return seed;
	}
};

class RTModel_Triangles {
	std::vector<SceneTypes::CPU::Triangle> triangles;
	SceneTypes::GPU::Material material;
	
public:
	RTModel_Triangles(std::vector<SceneTypes::CPU::Triangle>&& t, SceneTypes::GPU::Material mat);

	auto getTriangles() const -> const std::vector<SceneTypes::CPU::Triangle>&;
	auto getMaterialType() const -> SceneTypes::GPU::Material;
};

class RTModel_Sphere {
	glm::vec3 center;
	f32 radius;
	SceneTypes::GPU::Material material;

public:
	RTModel_Sphere(f32 radius, SceneTypes::GPU::Material mat); // assume center = 0,0,0
	//RTModel_Sphere(glm::vec3 center, f32 radius, SceneTypes::GPU::Material mat);

	auto getCenter() const -> const glm::vec3&;
	auto getRadius() const -> f32;
	auto getMaterialType() const -> SceneTypes::GPU::Material;
};

using RTModel = std::variant<RTModel_Triangles, RTModel_Sphere>;

auto loadModel(const std::string& filepath, glm::vec3 color) -> std::unique_ptr<RTModel>;
auto loadModel(const std::string& filepath, SceneTypes::GPU::Material mat) -> std::unique_ptr<RTModel>;
auto loadModel(f32 radius, SceneTypes::GPU::Material mat) -> std::unique_ptr<RTModel>;
