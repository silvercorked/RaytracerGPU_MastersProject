#pragma once

#include "../utils/PrimitiveTypes.hpp"

#include "Window.hpp"
#include "Camera.hpp"
#include "GameObject.hpp"
#include "KeyboardMovementController.hpp"

/*
i suppose, in theory, the camera and keyboard movement controller should be abstracted to components
and then can have a unique factory function like for pointlights on game object to construct one
but the api would be a bit uglier in that case. This is less flexible, but works for my needs. Going with this for simplicity for now.
*/

class CameraGameObject : public GameObject {
	Camera camera;
	KeyboardMovementController cameraController;
	f32 fovy;
	f32 nearDist; // used for normal rendering
	f32 farDist;  // used for normal rendering

	CameraGameObject(GameObjectId id);
public:
	static auto makeCameraGameObject() -> CameraGameObject;

	auto getProjection() const -> const glm::mat4&;
	auto getView() const -> const glm::mat4&;
	auto getInverseView() const -> const glm::mat4&;
	auto getPosition() const -> const glm::vec3;
	auto getDirection() const -> const glm::vec3;

	auto getVerticalFOV() const -> f32;
	auto setVerticalFOV(f32 fovy) -> void;

	auto updateCameraForFrame(Window& window, f32 dt, f32 aspectRatio) -> void;
};
