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

export module VulkanWrap:Model;

import :Device;
import :Buffer;
import :Utils;

import <vector>;
import <memory>;
import <iostream>;
import <unordered_map>;

/*
	Take vertex data from cpu, allocate memory and copy data over to device gpu
*/
export class Model {
	Device& device;

	std::unique_ptr<Buffer> vertexBuffer;
	uint32_t vertexCount;

	bool hasIndexBuffer = false;			// optional index buffer support. just leave input builder's indices member empty if not desired
	std::unique_ptr<Buffer> indexBuffer;	// this allows the vertex buffer to only contain unique vertices (and any associated data, like color) // index buffers list indices of vertices in the vertex buffer
	uint32_t indexCount;					// and avoids copying vertex data to form each triangle

	/*
	 v1 ______ v2/v4
		|   /|
		|  / |
		| /  |
  v3/v5 |/___| v6
		vertexBuffer (no indexbuffer) = {v1, v2, v3, v4, v5, v6}

		vertexBuffer (w/ indexbuffer) = {v1, v2, v3, v6}
		indexBuffer = {0, 1, 2, 1, 2, 3}
		0, 1, 2 => v1, v2, v3. 1, 2, 3 => v2, v3, v6.

		With complex models, it's feasible to have tons of shared vertices. Index buffers reduce memory usage as models get more complex
	*/

public:
	struct Vertex {
		glm::vec3 position;
		glm::vec3 color;
		glm::vec3 normal{};
		glm::vec2 uv{};

		static auto getBindingDescriptions() -> std::vector<VkVertexInputBindingDescription>;
		static auto getAttributeDescriptions() -> std::vector<VkVertexInputAttributeDescription>;

		bool operator==(const Vertex& other) const {
			return this->position == other.position
				&& this->color == other.color
				&& this->normal == other.normal
				&& this->uv == other.uv;
		}
	};

	struct Builder {
		std::vector<Vertex> vertices{};
		std::vector<uint32_t> indices{};

		auto loadModel(const std::string& filepath) -> void;
	};

	Model(Device& device, const Model::Builder& builder);
	~Model();

	Model(const Model&) = delete;
	Model& operator=(const Model&) = delete;

	static auto createModelFromFile(Device& device, const std::string& filepath) -> std::unique_ptr<Model>;

	auto bind(VkCommandBuffer commandBuffer) -> void;
	auto draw(VkCommandBuffer commandBuffer) -> void;

private:
	auto createVertexBuffers(const std::vector<Vertex>& vertices) -> void;
	auto createIndexBuffers(const std::vector<uint32_t>& indices) -> void;
};

namespace std { // p sure this is undefined behavior
	template<>
	struct hash<Model::Vertex> {
		size_t operator()(Model::Vertex const& vertex) const {
			size_t seed = 0;
			hashCombine(seed, vertex.position, vertex.color, vertex.normal, vertex.uv);
			return seed;
		}
	};
}
