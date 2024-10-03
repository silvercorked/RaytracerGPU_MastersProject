#pragma once

#include "../../utils/PrimitiveTypes.hpp"
#include <type_traits>

struct PointLightComponent {
	f32 lightIntensity;

	PointLightComponent() : lightIntensity{ 1.0f } {}
	PointLightComponent(f32 intensity) : lightIntensity{ intensity } {}

	PointLightComponent(const PointLightComponent&) = default;
	PointLightComponent& operator=(const PointLightComponent&) = default;

	PointLightComponent(PointLightComponent&& p) noexcept :
		lightIntensity{ std::move(p.lightIntensity) }
	{};
	PointLightComponent& operator=(PointLightComponent&& p) noexcept {
		this->lightIntensity = std::move(p.lightIntensity);
	};
};

static_assert(std::is_copy_constructible_v<PointLightComponent>);
