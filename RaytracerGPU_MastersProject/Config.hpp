#pragma once

#include "utils/PrimitiveTypes.hpp"

namespace Config {
	enum struct Programs {
		LogisticMap,
		Raytracer
	};

	constexpr const Programs CurrentProgram = Programs::Raytracer;

	constexpr const bool ShowBufferDebug = 0;
	constexpr const bool Fake1SecondDelay = 0;
};
