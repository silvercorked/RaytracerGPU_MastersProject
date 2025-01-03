
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
	std::shared_ptr<RTModel> vase = loadModel("models/smooth_vase.obj", glm::vec3(1.0f, 0.1f, 1.0f));
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
	vaseGO.transform.translation = { 4.0f, 0.0f, 10.0f };
	vaseGO.transform.scale = { 5.0f, 5.0f, 5.0f };
	vaseGO.transform.rotation = { glm::pi<f32>(), 0.0f, 0.0f };

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

	scene->setMaxRaytraceDepth(100);
	scene->getCamera().setVerticalFOV(80.0f);
	scene->prepForRender();
}

auto cornellBoxScene(std::unique_ptr<RaytraceScene>& scene) -> void {
	SceneTypes::Material redDiff(glm::vec3(0.65, 0.05, 0.05), SceneTypes::MaterialType::DIFFUSE);
	SceneTypes::Material whiteDiff(glm::vec3(0.73, 0.73, 0.73), SceneTypes::MaterialType::DIFFUSE);
	SceneTypes::Material greenDiff(glm::vec3(0.12, 0.45, 0.15), SceneTypes::MaterialType::DIFFUSE);
	SceneTypes::Material light(glm::vec3(15.0, 15.0, 15.0), SceneTypes::MaterialType::LIGHT);

	std::shared_ptr<RTModel> quadRed = loadModel("models/quad.obj", redDiff);
	std::shared_ptr<RTModel> quadGreen = loadModel("models/quad.obj", greenDiff);
	std::shared_ptr<RTModel> quadWhite = loadModel("models/quad.obj", whiteDiff);
	std::shared_ptr<RTModel> quadLight = loadModel("models/quad.obj", light);
	std::shared_ptr<RTModel> cubeWhite = loadModel("models/cube.obj", whiteDiff); // might be nice to add a duplicate & setMaterial function to RTModels so allow loading model files once.

	TransformComponent t;
	t.translation = { 0.0f, 0.0f, 0.0f };
	t.scale = { 275.0f, 1.0f, 275.0f };
	t.rotation = { 0.0f, 0.0f, 0.0f };

	GameObject leftWall = GameObject::createGameObject();
	leftWall.setModel(quadGreen, true);
	leftWall.transform = t; // copy
	leftWall.transform.translation = { 550.0f, 275.0f, 275.0f };
	leftWall.transform.rotation = { 0, 0, glm::pi<f32>() / 2 };
	
	GameObject rightWall = GameObject::createGameObject();
	rightWall.setModel(quadRed, true);
	rightWall.transform = t; // copy
	rightWall.transform.translation = { 0.0f, 275.0f, 275.0f };
	rightWall.transform.rotation = { 0, 0, glm::pi<f32>() / 2 };
	
	GameObject roomLight = GameObject::createGameObject();
	roomLight.setModel(quadLight, true);
	roomLight.transform.translation = { 275.0f, 549.0f, 300.0f };
	roomLight.transform.scale = { 65.0f, 1.0f, 50.0f };
	roomLight.transform.rotation = { 0, 0, 0 };

	GameObject bottomWall = GameObject::createGameObject();
	bottomWall.setModel(quadWhite, true);
	bottomWall.transform = t; // copy
	bottomWall.transform.translation = { 275.0f, 0.0f, 275.0f };
	bottomWall.transform.rotation = { 0, 0, 0 };

	GameObject topWall = GameObject::createGameObject();
	topWall.setModel(quadWhite, true);
	topWall.transform = t;
	topWall.transform.translation = { 275.0f, 550.0f, 275.0f };

	GameObject backWall = GameObject::createGameObject();
	backWall.setModel(quadWhite, true);
	backWall.transform = t;
	backWall.transform.translation = { 275.0f, 275.0f, 550.0f };
	backWall.transform.rotation = { glm::pi<f32>() / 2, 0, 0 };

	scene->addGameObject(std::move(leftWall));
	scene->addGameObject(std::move(rightWall));
	scene->addGameObject(std::move(roomLight));
	scene->addGameObject(std::move(bottomWall));
	scene->addGameObject(std::move(topWall));
	scene->addGameObject(std::move(backWall));

	GameObject cube1 = GameObject::createGameObject();
	cube1.setModel(cubeWhite, true);
	cube1.transform.translation = { 350.0f, 160.f, 395.0f };
	cube1.transform.scale = { 80.0f, 160.0f, 80.0f };
	cube1.transform.rotation = { 0, glm::radians(15.0f), 0};

	GameObject cube2 = GameObject::createGameObject();
	cube2.setModel(cubeWhite, true);
	cube2.transform.translation = { 180.0f, 160.0f, 175.0f };
	cube2.transform.scale = { 80.0f, 80.0f, 80.0f };
	cube2.transform.rotation = { 0, glm::radians(-18.0f), 0}; // glm::radians(45.0f), glm::radians(-45.0f), glm::radians(45.0f)

	scene->addGameObject(std::move(cube1));
	scene->addGameObject(std::move(cube2));

	std::shared_ptr<RTModel> sphere = loadModel(1.0f, whiteDiff);
	GameObject dummySphere = GameObject::createGameObject();
	dummySphere.setModel(sphere, false);
	dummySphere.transform.translation = { -200.0f, 215.0f, -50.0f }; // requires 1 sphere min rn, so just throw this out of the way
	scene->addGameObject(std::move(dummySphere));

	scene->setRaysPerPixel(128);
	scene->setMaxRaytraceDepth(25);
	scene->getCamera().setVerticalFOV(40.0f);
	scene->prepForRender();
}

auto simpleScene(std::unique_ptr<RaytraceScene>& scene) -> void {
	SceneTypes::Material light(glm::vec3(15.0, 15.0, 15.0), SceneTypes::MaterialType::LIGHT);
	SceneTypes::Material whiteDiff(glm::vec3(0.73, 0.73, 0.73), SceneTypes::MaterialType::DIFFUSE);
	SceneTypes::Material redDiff(glm::vec3(0.65, 0.05, 0.05), SceneTypes::MaterialType::DIFFUSE);
	std::shared_ptr<RTModel> quadLight = loadModel("models/quad.obj", light);
	std::shared_ptr<RTModel> quadWhite = loadModel("models/quad.obj", whiteDiff);
	std::shared_ptr<RTModel> quadRed = loadModel("models/quad.obj", redDiff);

	TransformComponent t;
	t.translation = { 0.0f, 0.0f, 0.0f };
	t.scale = { 275.0f, 1.0f, 275.0f };
	t.rotation = { 0.0f, 0.0f, 0.0f };

	GameObject roomLight = GameObject::createGameObject();
	roomLight.setModel(quadLight, true);
	roomLight.transform.translation = { 275.0f, 549.0f, 300.0f };
	roomLight.transform.scale = { 65.0f, 1.0f, 50.0f };
	roomLight.transform.rotation = { 0, 0, 0 };

	GameObject bottomWall = GameObject::createGameObject();
	bottomWall.setModel(quadWhite, true);
	bottomWall.transform = t; // copy
	bottomWall.transform.translation = { 275.0f, 0.0f, 275.0f };
	bottomWall.transform.rotation = { 0, 0, 0 };

	GameObject backWall = GameObject::createGameObject();
	backWall.setModel(quadRed, true);
	backWall.transform = t;
	backWall.transform.translation = { 275.0f, 275.0f, 550.0f };
	backWall.transform.rotation = { glm::pi<f32>() / 2, 0, 0 };

	std::shared_ptr<RTModel> sphere = loadModel(200.0f, whiteDiff);
	GameObject dummySphere = GameObject::createGameObject();
	dummySphere.setModel(sphere, false);
	dummySphere.transform.translation = { 100.0f, 445.0f, 215.0f };
	
	scene->addGameObject(std::move(roomLight));
	scene->addGameObject(std::move(bottomWall));
	scene->addGameObject(std::move(backWall));
	scene->addGameObject(std::move(dummySphere));

	scene->setRaysPerPixel(16);
	scene->setMaxRaytraceDepth(8);
	scene->getCamera().setVerticalFOV(40.0f);
	scene->prepForRender();
}

auto complexScene(std::unique_ptr<RaytraceScene>& scene) -> void {
	SceneTypes::Material light(glm::vec3(15.0, 15.0, 15.0), SceneTypes::MaterialType::LIGHT);
	SceneTypes::Material whiteDiff(glm::vec3(0.33, 0.73, 0.33), SceneTypes::MaterialType::DIFFUSE);
	SceneTypes::Material redDiff(glm::vec3(0.65, 0.05, 0.05), SceneTypes::MaterialType::DIFFUSE);
	SceneTypes::Material blueDiff(glm::vec3(0.12, 0.15, 0.45), SceneTypes::MaterialType::DIFFUSE);
	std::shared_ptr<RTModel> quadLight = loadModel("models/quad.obj", light);
	std::shared_ptr<RTModel> quadWhite = loadModel("models/quad.obj", whiteDiff);
	std::shared_ptr<RTModel> monkeyRed = loadModel("models/monkey.obj", redDiff);
	std::shared_ptr<RTModel> monkeyBlue = loadModel("models/monkey.obj", blueDiff);

	TransformComponent t;
	t.translation = { 0.0f, 0.0f, 0.0f };
	t.scale = { 275.0f, 1.0f, 275.0f };
	t.rotation = { 0.0f, 0.0f, 0.0f };

	GameObject roomLight = GameObject::createGameObject();
	roomLight.setModel(quadLight, true);
	roomLight.transform.translation = { 275.0f, 549.0f, 300.0f };
	roomLight.transform.scale = { 65.0f, 1.0f, 50.0f };
	roomLight.transform.rotation = { 0, 0, 0 };

	GameObject bottomWall = GameObject::createGameObject();
	bottomWall.setModel(quadWhite, true);
	bottomWall.transform = t; // copy
	bottomWall.transform.translation = { 275.0f, 0.0f, 275.0f };
	bottomWall.transform.rotation = { 0, 0, 0 };

	GameObject backWall = GameObject::createGameObject();
	backWall.setModel(quadWhite, true);
	backWall.transform = t;
	backWall.transform.translation = { 275.0f, 275.0f, 550.0f };
	backWall.transform.rotation = { glm::pi<f32>() / 2, 0, 0 };

	std::shared_ptr<RTModel> sphere = loadModel(40.0f, redDiff);
	GameObject dummySphere = GameObject::createGameObject();
	dummySphere.setModel(sphere, false);
	dummySphere.transform.translation = { 100.0f, 215.0f, 50.0f }; // 200.0f, 215.0f, 50.0f

	GameObject monkey1 = GameObject::createGameObject();
	monkey1.setModel(monkeyRed, true);
	monkey1.transform.translation = { 375.0f, 375.0f, 275.0f };
	monkey1.transform.scale = { 100.0f, 100.0f, 100.0f };
	monkey1.transform.rotation = { glm::radians(-35.0f), glm::radians(180.0f), glm::radians(15.0f) };

	GameObject monkey2 = GameObject::createGameObject();
	monkey2.setModel(monkeyBlue, true);
	monkey2.transform.translation = { 175.0f, 125.0f, 275.0f };
	monkey2.transform.scale = { 100.0f, 100.0f, 100.0f };
	monkey2.transform.rotation = { glm::radians(-36.0f), glm::radians(180.0f), glm::radians(21.0f) };

	scene->addGameObject(std::move(roomLight));
	scene->addGameObject(std::move(bottomWall));
	scene->addGameObject(std::move(backWall));
	//scene->addGameObject(std::move(monkey1));
	scene->addGameObject(std::move(monkey2));
	scene->addGameObject(std::move(dummySphere));

	scene->setRaysPerPixel(8);
	scene->setMaxRaytraceDepth(8);
	scene->getCamera().setVerticalFOV(40.0f);
	scene->prepForRender();
}
