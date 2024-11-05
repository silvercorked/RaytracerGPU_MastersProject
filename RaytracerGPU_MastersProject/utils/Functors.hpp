#pragma once

#include "PrimitiveTypes.hpp"
#include "Concepts.hpp"

#include <type_traits>

namespace Util {
	constexpr const auto pickAndSave = []<
		Numeric N,
		FuncTaking2NumericReturningNumeric<N> F
	>(
		N& a,
		N b,
		F pickingFunc
	) -> void {
		a = pickingFunc(a, b);
	};
};
