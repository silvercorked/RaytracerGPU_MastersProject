
#include "KeyboardMovementController.hpp"

auto KeyboardMovementController::moveInPlaneXZ(GLFWwindow* window, float dt, GameObject& gameObject) -> void {
	glm::vec3 rotate{ 0 };
	if (glfwGetKey(window, keys.lookRight) == GLFW_PRESS) // check if key is being pressed
		rotate.y += 1.0f;
	if (glfwGetKey(window, keys.lookLeft) == GLFW_PRESS)
		rotate.y -= 1.0f;
	if (glfwGetKey(window, keys.lookUp) == GLFW_PRESS)
		rotate.x += 1.0f;
	if (glfwGetKey(window, keys.lookDown) == GLFW_PRESS)
		rotate.x -= 1.0f;

	// vary by time step to ensure similar simulation w/ diff framerates
	// normalize rotate to avoid faster rotation in "diagonal" situations (ie, ||{1, 1, 0}|| = ||{1, 0, 0}||)
	// have to check for zero tho cause normalize cant occur on a zero vector
	if (glm::dot(rotate, rotate) > std::numeric_limits<float>::epsilon()) // ||v||^2 >= 0
		gameObject.transform.rotation += this->lookSpeed * dt * glm::normalize(rotate);

	// limit pitch values between +/- 85-ish degrees to avoid turning vertically upside down
	gameObject.transform.rotation.x = glm::clamp(gameObject.transform.rotation.x, -1.5f, 1.5f);
	gameObject.transform.rotation.y = glm::mod(gameObject.transform.rotation.y, glm::two_pi<float>()); // avoid repeated spinning

	/*
						-x   -y   +z
						\   |   /
						 \  |  /
						  \ | /yaw
						   \|/-------> forward direction
						   /|\
						  / | \
						 /  |  \
						/   |   \
						-z   +y   +x
	*/

	float yaw = gameObject.transform.rotation.y; // rotation from +z axis to forward dir about y axis
	const glm::vec3 forwardDir{ sin(yaw), 0.0f, cos(yaw) };
	const glm::vec3 rightDir{ forwardDir.z, 0.0f, -forwardDir.x }; // right angle to forward dir in xz plane
	const glm::vec3 upDir{ 0.0f, -1.0f, 0.0f }; // right angle to forward and right

	glm::vec3 moveDir{ 0.0f };
	if (glfwGetKey(window, keys.moveForward) == GLFW_PRESS)
		moveDir += forwardDir;
	if (glfwGetKey(window, keys.moveBackward) == GLFW_PRESS)
		moveDir -= forwardDir;
	if (glfwGetKey(window, keys.moveLeft) == GLFW_PRESS)
		moveDir -= rightDir;
	if (glfwGetKey(window, keys.moveRight) == GLFW_PRESS)
		moveDir += rightDir;
	if (glfwGetKey(window, keys.moveUp) == GLFW_PRESS)
		moveDir += upDir;
	if (glfwGetKey(window, keys.moveDown) == GLFW_PRESS)
		moveDir -= upDir;

	if (glm::dot(moveDir, moveDir) > std::numeric_limits<float>::epsilon()) // ||v||^2 >= 0
		gameObject.transform.translation += this->moveSpeed * dt * glm::normalize(moveDir);
}
