#pragma once

#include "utils/PrimitiveTypes.hpp"

namespace Config {
	enum struct Programs {
		LogisticMap,
		Raytracer,
		RaytracerBVH
	};

	constexpr const Programs CurrentProgram = Programs::RaytracerBVH;

	constexpr const bool ShowBufferDebug = 0;
	constexpr const bool Fake1SecondDelay = 0;

	constexpr const bool RunRayPerPixelIncreasingDemo = 0;
	namespace RayPerPixelIncreasingDemoConfig {
		constexpr const u32 runsBeforeIncrease = 4;
		constexpr const u32 startRaysPerPixel = 100;
		constexpr const u32 maxRaysPerPixel = 200;
		constexpr const u32 increaseAmount = 5;
	};
};
