#pragma once

#include <type_traits>

namespace Util {
	template <typename I>
	concept Integral = std::is_integral_v<I>;
	template <typename F>
	concept Floating = std::is_floating_point_v<F>;
	template <typename N>
	concept Numeric = Integral<N> || Floating<N>;

	template <typename F, typename N>
	concept FuncTaking2NumericReturningNumeric = Numeric<N> && std::is_invocable_r_v<N, F, N, N>;
};