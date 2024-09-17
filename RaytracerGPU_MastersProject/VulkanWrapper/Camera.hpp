#pragma once

#define GLM_FORCE_RADIANS					// functions expect radians, not degrees
#define GLM_FORCE_DEPTH_ZERO_TO_ONE			// Depth buffer values will range from 0 to 1, not -1 to 1
#include <glm/glm.hpp>

class Camera {
	glm::mat4 projectionMatrix{ 1.0f };
	glm::mat4 viewMatrix{ 1.0f };
	glm::mat4 inverseViewMatrix{ 1.0f };

public:
	auto setOrthographicProjection(float left, float right, float top, float bottom, float near, float far) -> void;
	auto setPerspectiveProjection(float fovy, float aspect, float near, float far) -> void;

	auto setViewDirection(glm::vec3 position, glm::vec3 dir, glm::vec3 up = glm::vec3{ 0.0f, -1.0f, 0.0f }) -> void;
	auto setViewTarget(glm::vec3 position, glm::vec3 target, glm::vec3 up = glm::vec3{ 0.0f, -1.0f, 0.0f }) -> void;
	auto setViewYXZ(glm::vec3 position, glm::vec3 rotation) -> void;

	auto getProjection() const -> const glm::mat4& { return this->projectionMatrix; }
	auto getView() const -> const glm::mat4& { return this->viewMatrix; }
	auto getInverseView() const -> const glm::mat4& { return this->inverseViewMatrix; }
	auto getPosition() const -> const glm::vec3 { return glm::vec3(this->inverseViewMatrix[3]); }
	auto getDirection() const -> const glm::vec3 { return glm::vec3(this->inverseViewMatrix[2]); } // i think this works
};
