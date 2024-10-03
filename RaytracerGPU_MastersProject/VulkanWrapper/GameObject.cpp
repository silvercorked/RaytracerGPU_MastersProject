
#include "GameObject.hpp"

#include <cassert>

auto GameObject::createGameObject() -> GameObject {
	static GameObjectId currentId = 1;
	return GameObject{ currentId++ };
}

auto GameObject::makePointLight(f32 intensity, f32 radius, glm::vec3 color) -> GameObject {
	GameObject gameObj = GameObject::createGameObject();
	gameObj.color = color;
	gameObj.transform.scale.x = radius;

	size_t index = gameObj.addComponent<PointLightComponent>();
	gameObj.getComponent<PointLightComponent>(index)->lightIntensity = intensity;

	return gameObj;
}

auto GameObject::setModel(std::shared_ptr<RTModel> model, bool triangular) -> void {
	assert(
		triangular
		? std::holds_alternative<RTModel_Triangles>(*model)
		: std::holds_alternative<RTModel_Sphere>(*model)
	);
	if (triangular) {
		this->isTriangleModel = true;
		this->isSphereModel = false;
	}
	else {
		this->isTriangleModel = false;
		this->isSphereModel = true;
	}
	this->model = model;
}
auto GameObject::getModel() const -> std::shared_ptr<RTModel> {
	return this->model;
}

/* 
* vector of unique ptr cant be trivially copied(cause unique ptr has deleted copy constructor), 
* but copying game objects is kinda of useful
* So, do normal copy for all but component vector. Then constructor copies (via component copy constructors) and create new unique ptrs
* for the copies to create a new gameobject. Id needs to be different too, so snag a new one
*/
GameObject::GameObject(const GameObject& obj) :
	model{ obj.model },
	color{ obj.color },
	transform{ obj.transform },
	isTriangleModel{ obj.isTriangleModel },
	isSphereModel{ obj.isSphereModel },
	components{},
	id{ GameObject::createGameObject().id } // grab a unqiue id
{
	this->components.reserve(obj.components.size());
	for (auto i = 0; i < obj.components.size(); i++) {
		std::visit([&nComponents = this->components](auto& arg) {
			using T = std::decay_t<decltype(*arg)>; // get type inside unique ptr
			nComponents.push_back(
				std::make_unique<T>(*arg) // should be picking copy constructor on component, not move
			); // push back a copy-constructed version of contents of uniqueptr
		}, obj.components[i]);
	}
}

auto GameObject::getId() const -> GameObjectId {
	return this->id;
}
