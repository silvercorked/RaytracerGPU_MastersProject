#pragma once

#include "../utils/PrimitiveTypes.hpp"

#include "components/TransformComponent.hpp"
#include "components/PointLightComponent.hpp"
#include "RTModel.hpp"

#include <memory>
#include <unordered_map>
#include <variant>
#include <stdexcept>

/*
variant vs inheritance for Components
Inheritance:
- easy concept creation (get to use derived_from to include any newely made components easily
- difficult access/search methods. need to rely on static cast and check for erronous casts or dynamic cast
 - dynamic cast requires virtual functions on base class, meaning component may need unnecessary definitions
variant:
- difficult concept creation (have to rely on same as, meaning new components need to be added to the list each time)
- easy access/search methods. get to rely on std::get, std visit, and std holds alternative and their error checking.

can't use both (ie, variant for underlying storage type and inheritance for concept creation. because variant requires a type list,
so u'd bite the bullet on variants downside anyway. 
*/

template<typename... Types>
struct TypeList {};

template <typename T, typename List>
concept IsAnyOf = []<typename... Types>(TypeList<Types...>) {
	return (std::is_same_v<Types, T> || ...);
}(List());

using ComponentsTypeList = TypeList<
	TransformComponent, PointLightComponent
>;

template <typename C>
concept ComponentType = IsAnyOf<C, ComponentsTypeList>;

template <typename ... Types> // using function and decltype to get type. not implemented and not expected to ever be called
std::variant<std::unique_ptr<Types>...> asVariantUniquePtrs(TypeList<Types...>); // wrap all types in unique ptrs and make a variant of them

using ComponentVariantType = decltype(asVariantUniquePtrs(ComponentsTypeList{}));

using ComponentsVector = std::vector<ComponentVariantType>;

using GameObjectId = u32;

struct GameObject {
	using Map = std::unordered_map<GameObjectId, GameObject>;

protected:
	GameObjectId id;
	std::shared_ptr<RTModel> model{};

public:
	static auto createGameObject() -> GameObject;

	static auto makePointLight(
		f32 intensity = 10.0f,
		f32 radius = 0.1f,
		glm::vec3 color = glm::vec3(1.0f)
	) -> GameObject;

	template <ComponentType C>
	static auto getComponent(GameObject& obj, size_t index) -> C*; // return pointers to let others interact, but dont give any lifetime expectations
	template <ComponentType C>
	static auto addComponent(GameObject& obj) -> size_t;

	auto setModel(std::shared_ptr<RTModel> model, bool triangular) -> void;
	auto getModel() const -> std::shared_ptr<RTModel>;

	template <ComponentType C>
	auto getComponent(size_t index) const -> C*;
	template <ComponentType C>
	auto addComponent() -> size_t;

	template <ComponentType C>
	auto getComponent() const -> C*;
	template <ComponentType C>
	auto getComponents() const -> std::vector<C*>;

	GameObject(const GameObject&);
	GameObject& operator=(const GameObject&) = delete;

	GameObject(GameObject&&) = default;
	GameObject& operator=(GameObject&&) = default;

	auto getId() const -> GameObjectId;

	glm::vec3 color{};
	TransformComponent transform{};

	ComponentsVector components;

	bool isTriangleModel;
	bool isSphereModel;

protected:
	GameObject(GameObjectId objId) : id{ objId }, isTriangleModel(false), isSphereModel(false) {}
};

template <ComponentType C>
inline auto GameObject::getComponent(GameObject& obj, size_t index) -> C* {
	return obj.getComponent<C>(index);
}

template <ComponentType C>
inline auto GameObject::addComponent(GameObject& obj) -> size_t {
	return obj.addComponent<C>();
}

template <ComponentType C>
inline auto GameObject::getComponent(size_t index) const -> C* {
	if (index <= this->components.size()) {
		if (
			std::holds_alternative<std::unique_ptr<C>>(this->components[index])
			) { // this->components[index].operator->() doesnt work for some reason, unsure
			return std::get<std::unique_ptr<C>>(this->components[index]).get();
		}
		return nullptr;
	}
	throw std::out_of_range("Invalid index for component!");
}

template <ComponentType C>
inline auto GameObject::addComponent() -> size_t {
	this->components.emplace_back(std::make_unique<C>());
	return this->components.size() - 1; // should be the last one
}

template<ComponentType C>
inline auto GameObject::getComponent() const -> C* {
	for (auto& component : this->components) {
		if (
			std::holds_alternative<std::unique_ptr<C>>(component)
		) {
			return std::get<std::unique_ptr<C>>(component).get();
		}
	}
	return nullptr;
}

template<ComponentType C>
inline auto GameObject::getComponents() const -> std::vector<C*> {
	std::vector<C*> comps;
	for (auto& component : this->components) {
		if (
			std::holds_alternative<std::unique_ptr<C>>(component)
		) {
			comps.push_back(
				std::get<std::unique_ptr<C>>(component).get()
			);
		}
	}
	return comps;
}
