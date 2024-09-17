#pragma once

#include <type_traits>
#include <variant>
#include <memory>

// from: https://stackoverflow.com/a/57595105
template <typename T, typename... Rest>
void hashCombine(std::size_t& seed, const T& v, const Rest&... rest) {
	seed ^= std::hash<T>{}(v)+0x9e3779b9 + (seed << 6) + (seed >> 2);
	(hashCombine(seed, rest), ...);
};

template <typename C, typename ...Types>
auto getVariantFromSharedPtr(std::shared_ptr<std::variant<Types...>> v) -> C* {
	if (std::holds_alternative<C>(*v))
		return &std::get<C>(*v);
	return nullptr;
}
