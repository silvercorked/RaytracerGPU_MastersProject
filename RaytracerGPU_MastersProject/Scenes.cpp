
#include "Scenes.hpp"

#include "VulkanWrapper/RTModel.hpp"
#include "VulkanWrapper/GameObject.hpp"

#include <random>

std::random_device rd;
std::uniform_real_distribution<double> dist(0, 1.0);
std::mt19937 gen{ rd() };

auto randomDouble = [](f32 min = 0.0, f32 max = 1.0) -> f32 {
	return min + (max - min) * dist(gen);
};
auto randomColor = [](f32 min = 0.0, f32 max = 1.0) -> glm::vec3 {
	return glm::vec3(randomDouble(min, max), randomDouble(min, max), randomDouble(min, max));
};

auto randomSpheres(std::unique_ptr<RaytraceScene>& scene) -> void {
	auto groundMaterial = SceneTypes::GPU::Material(glm::vec3(0.5, 0.5, 0.5), SceneTypes::MaterialType::DIFFUSE);
	auto floor = GameObject::createGameObject();
	floor.setModel(loadModel(1000, groundMaterial), false);
	floor.transform.translation = { 0, -1000, 0 };
	scene->addGameObject(std::move(floor));


	/*
	for (int a = -11; a < 11; a++) {
		for (int b = -11; b < 11; b++) {
			auto chooseMat = randomDouble();
			glm::vec3 center(a + 0.9 * randomDouble(), 0.2, b + 0.9 * randomDouble());
			if ((center - glm::vec3(4, 0.2, 0)).length() > 0.9) {
				SceneTypes::GPU::Material sphereMat;
				if (chooseMat < 0.8) { // Diffuse
					auto albedo = randomColor() * randomColor();
					sphereMat.albedo = albedo;
					sphereMat.materialType = SceneTypes::MaterialType::DIFFUSE;
					auto obj = GameObject::createGameObject();
					obj.setModel(loadModel(0.2, sphereMat), false);
					obj.transform.translation = center;
					scene->addGameObject(std::move(obj));
				}
				else if (chooseMat < 0.95) { // metal
					auto albedo = randomColor();
					auto fuzz = randomDouble(0, 0.5);
					sphereMat.albedo = albedo;
					sphereMat.materialType = SceneTypes::MaterialType::METALLIC;
					auto obj = GameObject::createGameObject();
					obj.setModel(loadModel(0.2, sphereMat), false);
					obj.transform.translation = center;
					scene->addGameObject(std::move(obj));
				}
				else { // glass
					sphereMat.albedo = randomColor();
					sphereMat.materialType = SceneTypes::MaterialType::DIELECTRIC;
					auto obj = GameObject::createGameObject();
					obj.setModel(loadModel(0.2, sphereMat), false);
					obj.transform.translation = center;
					scene->addGameObject(std::move(obj));
				}
			}
		}
	}
	*/

	auto mat1 = SceneTypes::GPU::Material({ 1.0f, 1.0f, 1.0f }, SceneTypes::MaterialType::LIGHT);
	auto mat2 = SceneTypes::GPU::Material(randomColor(), SceneTypes::MaterialType::DIFFUSE);
	auto mat3 = SceneTypes::GPU::Material(randomColor(), SceneTypes::MaterialType::METALLIC);
	auto mat4 = SceneTypes::GPU::Material(randomColor(), SceneTypes::MaterialType::DIFFUSE);
	auto obj1 = GameObject::createGameObject();
	auto obj2 = GameObject::createGameObject();
	auto obj3 = GameObject::createGameObject();
	auto obj4 = GameObject::createGameObject();
	auto obj5 = GameObject::createGameObject();
	auto obj6 = GameObject::createGameObject();
	auto obj7 = GameObject::createGameObject();
	auto obj8 = GameObject::createGameObject();
	obj1.transform.translation = { 0, 1.05f, 3.5f };
	obj1.setModel(loadModel(1.0, mat1), false);
	obj2.transform.translation = { -4, 1, 4 };
	obj2.setModel(loadModel(2.0, mat2), false);
	obj3.transform.translation = { 4, 1, 4 };
	obj3.setModel(loadModel(3.0, mat3), false);
	obj4.transform.translation = { 0, 1, -4 };
	obj4.setModel(loadModel(1.0, mat4), false);
	obj5.transform.translation = { 4, 1, -4 };
	obj5.setModel(loadModel(2.0, mat4), false);
	obj6.transform.translation = { -4, 1, -4 };
	obj6.setModel(loadModel(3.0, mat4), false);
	obj7.transform.translation = { 4, 5, 4 };
	obj7.setModel(loadModel(1.0, mat2), false);
	obj8.transform.translation = { -4, 5, 4 };
	obj8.setModel(loadModel(1.0, mat2), false);
	scene->addGameObject(std::move(obj1));
	scene->addGameObject(std::move(obj2));
	scene->addGameObject(std::move(obj3));
	scene->addGameObject(std::move(obj4));
	scene->addGameObject(std::move(obj5));
	scene->addGameObject(std::move(obj6));
	scene->addGameObject(std::move(obj7));
	scene->addGameObject(std::move(obj8));

	auto triObj = GameObject::createGameObject();
	triObj.setModel(loadModel("models/quad.obj", glm::vec3(0.3f, 0.5f, 0.7f)), true);
	triObj.transform.translation = { 0, 1.0f, 5.0f };
	triObj.transform.rotation = { 4.0f, 0, 0 };
	triObj.transform.scale = { 5.0f, 1.0f, 3.0f };
	scene->addGameObject(std::move(triObj));

	scene->setMaxRaytraceDepth(5);
	scene->getCamera().setVerticalFOV(90.0f);
	scene->prepForRender();
}

auto cornellMixedScene(std::unique_ptr<RaytraceScene>& scene) -> void {
	std::shared_ptr<RTModel> quadRed = loadModel("models/quad.obj", glm::vec3(1.0f, 0.1f, 0.1f));
	std::shared_ptr<RTModel> quadGreen = loadModel("models/quad.obj", glm::vec3(0.1f, 1.0f, 0.1f));
	std::shared_ptr<RTModel> quadBlue = loadModel("models/quad.obj", glm::vec3(0.1f, 0.1f, 1.0f));
	std::shared_ptr<RTModel> quadWhite = loadModel("models/quad.obj", glm::vec3(1.0f, 1.0f, 1.0f));
	std::shared_ptr<RTModel> vase = loadModel("models/smooth_vase.obj", glm::vec3(0.5f, 0, 0));
	std::shared_ptr<RTModel> cubeWhite = loadModel("models/cube.obj", glm::vec3(1.0f, 1.0f, 1.0f));
	SceneTypes::GPU::Material sphereMat{};
	sphereMat.albedo = { 1.0f, 1.0f, 1.0f };
	sphereMat.materialType = SceneTypes::MaterialType::LIGHT;

	auto floor = GameObject::createGameObject();
	floor.setModel(quadGreen, true);
	floor.transform.translation = { 0, -4.0f, 10.0f };
	floor.transform.scale = { 10.0f, 1.0f, 3.0f };
	floor.transform.rotation = { 0, 0, 0 };

	auto leftWall = GameObject::createGameObject();
	leftWall.setModel(quadWhite, true);
	leftWall.transform.translation = { -10.0f, 1.0f, 10.0f };
	leftWall.transform.scale = { 5.0f, 1.0f, 3.0f };
	leftWall.transform.rotation = { 0.0f, 0.0f, glm::pi<f32>() / 2 };

	auto rightWall = GameObject::createGameObject();
	rightWall.setModel(quadWhite, true);
	rightWall.transform.translation = { 10.0f, 1.0f, 10.0f };
	rightWall.transform.scale = { 5.0f, 1.0f, 3.0f };
	rightWall.transform.rotation = { 0.0f, 0.0f, -glm::pi<f32>() / 2 };

	auto roof = GameObject::createGameObject();
	roof.setModel(quadRed, true);
	roof.transform.translation = { 0.0f, 6.0f, 10.0f };
	roof.transform.scale = { 10.0f, 1.0f, 3.0f };
	roof.transform.rotation = { 0, 0, 0 };

	auto backWall = GameObject::createGameObject();
	backWall.setModel(quadBlue, true);
	backWall.transform.translation = { 0.0f, 1.0f, 13.0f };
	backWall.transform.scale = { 10.0f, 1.0f, 6.0f };
	backWall.transform.rotation = { glm::pi<f32>() / 2, 0.0f, 0.0f };

	auto vaseGO = GameObject::createGameObject();
	vaseGO.setModel(vase, true);
	vaseGO.transform.translation = { 1.0f, 0.0f, 5.0f };
	vaseGO.transform.scale = { 1.0f, 1.0f, 1.0f };
	vaseGO.transform.rotation = { 0.0f, 0.0f, 0.0f };

	auto sphere = GameObject::createGameObject();
	sphere.setModel(loadModel(0.5f, sphereMat), false);
	sphere.transform.translation = { 0.0f, 6.0f, 10.0f };
	sphere.transform.scale = { 1.0f, 1.0f, 1.0f };
	sphere.transform.rotation = { 0.0f, 0.0f, 0.0f };

	scene->addGameObject(std::move(floor));
	scene->addGameObject(std::move(leftWall));
	scene->addGameObject(std::move(rightWall));
	scene->addGameObject(std::move(roof));
	scene->addGameObject(std::move(backWall));
	//scene->addGameObject(std::move(vaseGO));
	scene->addGameObject(std::move(sphere));

	auto cube = GameObject::createGameObject();
	cube.setModel(cubeWhite, true);
	cube.transform.translation = { -5.0f, -2.0f, 10.0f };
	cube.transform.scale = { 2.0f, 2.0f, 2.0f };
	cube.transform.rotation = { 0, glm::pi<f32>() / 3, 0 };

	auto cube2 = GameObject::createGameObject();
	cube2.setModel(cubeWhite, true);
	cube2.transform.translation = { 4.0f, -2.0f, 10.0f };
	cube2.transform.scale = { 2.0f, 2.0f, 2.0f };
	cube2.transform.rotation = { 0, -glm::pi<f32>() / 4, 0 };

	scene->addGameObject(std::move(cube));
	scene->addGameObject(std::move(cube2));

	auto mat = SceneTypes::GPU::Material(glm::vec3(0.3f, 0.5f, 0.7f), SceneTypes::MaterialType::DIFFUSE);
	auto sphere1 = GameObject::createGameObject();
	sphere1.transform.translation = { -0.5f, 0.0f, 12.0f };
	sphere1.setModel(loadModel(2.0f, mat), false);

	scene->addGameObject(std::move(sphere1));

	scene->setMaxRaytraceDepth(1);
	scene->getCamera().setVerticalFOV(80.0f);
	scene->prepForRender();
}
