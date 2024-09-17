#pragma once

#include <glm/gtc/matrix_transform.hpp>

#include "../../utils/PrimitiveTypes.hpp"
#include <type_traits>

struct TransformComponent {
	glm::vec3 translation;			// position offset
	glm::vec3 scale;				// scale
	glm::vec3 rotation;				// rotation (radians)

	TransformComponent() : translation{}, scale{ 1.f, 1.f, 1.f }, rotation{} {}
	TransformComponent(glm::vec3 t, glm::vec3 s, glm::vec3 r) : translation{ t }, scale{ s }, rotation{ r } {}

	auto mat4() const -> glm::mat4;
	auto normalMatrix() const -> glm::mat3;

	TransformComponent(const TransformComponent&) = default;
	TransformComponent& operator=(const TransformComponent&) = default;

	TransformComponent(TransformComponent&& t) noexcept :
		translation{ std::move(t.translation) },
		scale{ std::move(t.scale) },
		rotation{ std::move(t.rotation) }
	{};
	TransformComponent& operator=(TransformComponent&& t) noexcept {
		this->translation = std::move(t.translation);
		this->scale = std::move(t.scale);
		this->rotation = std::move(t.rotation);
	};
};
