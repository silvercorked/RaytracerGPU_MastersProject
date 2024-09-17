
#include "RTModel.hpp"

#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

#include <stdexcept>
#include <unordered_map>

RTModel_Triangles::RTModel_Triangles(std::vector<SceneTypes::CPU::Triangle>&& t, SceneTypes::GPU::Material mat)
	: triangles{ t }, material{ mat }
{}

auto RTModel_Triangles::getTriangles() const -> const std::vector<SceneTypes::CPU::Triangle>& {
	return this->triangles;
}

auto RTModel_Triangles::getMaterialType() const -> SceneTypes::GPU::Material {
	return this->material;
}



RTModel_Sphere::RTModel_Sphere(f32 r, SceneTypes::GPU::Material mat)
	: center{ 0 }, radius{r}, material{mat}
{}

auto RTModel_Sphere::getCenter() const -> const glm::vec3& {
	return this->center;
}

auto RTModel_Sphere::getRadius() const -> f32 {
	return this->radius;
}

auto RTModel_Sphere::getMaterialType() const -> SceneTypes::GPU::Material {
	return this->material;
}

auto loadModel(const std::string& filepath, glm::vec3 color) -> std::unique_ptr<RTModel> {
	tinyobj::attrib_t attrib;
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;
	std::string warn, err;

	if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, filepath.c_str()))
		throw std::runtime_error(warn + err);

	std::vector<Vertex> vertices{};
	std::vector<size_t> indices{};

	std::unordered_map<Vertex, uint32_t> uniqueVertices{};
	for (const auto& shape : shapes) {
		for (const auto& index : shape.mesh.indices) {
			Vertex vertex{};
			if (index.vertex_index >= 0) {
				vertex.position = {
					attrib.vertices[3 * index.vertex_index + 0],
					attrib.vertices[3 * index.vertex_index + 1],
					attrib.vertices[3 * index.vertex_index + 2]
				};
				vertex.color = {
					attrib.colors[3 * index.vertex_index + 0],
					attrib.colors[3 * index.vertex_index + 1],
					attrib.colors[3 * index.vertex_index + 2]
				};
			}
			if (index.normal_index >= 0) {
				vertex.normal = {
					attrib.normals[3 * index.normal_index + 0],
					attrib.normals[3 * index.normal_index + 1],
					attrib.normals[3 * index.normal_index + 2]
				};
			}
			if (index.texcoord_index >= 0) {
				vertex.uv = {
					attrib.texcoords[2 * index.texcoord_index + 0],
					attrib.texcoords[2 * index.texcoord_index + 1]
				};
			}
			if (uniqueVertices.count(vertex) == 0) {
				uniqueVertices[vertex] = static_cast<uint32_t>(vertices.size());
				vertices.push_back(vertex);
			}
			indices.push_back(uniqueVertices[vertex]);
		}
	}
	SceneTypes::GPU::Material mat;
	mat.albedo = glm::vec4(color, 0.0f);
	mat.materialType = SceneTypes::MaterialType::DIFFUSE; // simple diffuse stuff for now
	std::vector<SceneTypes::CPU::Triangle> triangles;
	triangles.reserve(indices.size() / 3);
	for (auto i = 0; i < indices.size(); i += 3) {
		triangles.emplace_back(SceneTypes::CPU::Triangle{
			vertices[indices[i + 0]].position,
			vertices[indices[i + 1]].position,
			vertices[indices[i + 2]].position
			});
	}
	std::unique_ptr<RTModel> model = std::make_unique<RTModel>(
		RTModel_Triangles(std::move(triangles), mat)
	);
	return model;
}

auto loadModel(f32 radius, SceneTypes::GPU::Material mat) -> std::unique_ptr<RTModel> {
	return std::make_unique<RTModel>(RTModel_Sphere(radius, mat));
}
