#pragma once

#include "Concepts.hpp"

namespace Util {
	template <Numeric N>
	constexpr auto min(N a, N b) -> N { return a < b ? a : b; }
	template <Numeric N>
	constexpr auto max(N a, N b) -> N { return a > b ? a : b; }
};


