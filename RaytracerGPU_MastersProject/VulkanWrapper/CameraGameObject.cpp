
#include "CameraGameObject.hpp"

CameraGameObject::CameraGameObject(GameObjectId id) :
    GameObject{ id }, camera{}, cameraController{}, fovy{ 50 }, nearDist{ 0.1f }, farDist{ 1000.0f }
{}

auto CameraGameObject::makeCameraGameObject() -> CameraGameObject {
    auto gameObj = GameObject::createGameObject();
    return CameraGameObject(gameObj.getId()); // steal an id and decontruct game object. temp for now
}

auto CameraGameObject::getProjection() const -> const glm::mat4& {
    return this->camera.getProjection();
}

auto CameraGameObject::getView() const -> const glm::mat4& {
    return this->camera.getView();
}

auto CameraGameObject::getInverseView() const -> const glm::mat4& {
    return this->camera.getInverseView();
}

auto CameraGameObject::getPosition() const -> const glm::vec3 {
    return this->camera.getPosition();
}

auto CameraGameObject::getDirection() const -> const glm::vec3 {
    return this->camera.getDirection();
}

auto CameraGameObject::getVerticalFOV() const -> f32 {
    return this->fovy;
}

auto CameraGameObject::setVerticalFOV(f32 fovy) -> void {
    this->fovy = fovy;
}

auto CameraGameObject::updateCameraForFrame(Window& window, float dt, f32 aspectRatio) -> void {
    cameraController.moveInPlaneXZ(window.window(), dt, *this);
    this->camera.setViewYXZ(this->transform.translation, this->transform.rotation);

    this->camera.setPerspectiveProjection(glm::radians(this->fovy), aspectRatio, this->nearDist, this->farDist);
}
