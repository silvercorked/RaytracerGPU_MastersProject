module;

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

export module VulkanWrap:Window;

import <string>;

export class Window {
	GLFWwindow* _window;
	int _width;
	int _height;
	bool _framebufferResized;
	std::string _windowName;

	static auto frambufferResizeCallback(GLFWwindow*, int, int) -> void;
	auto initWindow() -> void;
public:
	Window(int, int, std::string);
	~Window();

	Window(const Window&) = delete;					// copy and assignment deleted
	Window& operator=(const Window&) = delete;		// for RAII

	auto shouldClose() -> bool;
	auto getExtent() -> VkExtent2D;
	auto wasWindowResized() -> bool;
	auto resetWindowResizeFlag() -> void;
	auto createWindowSurface(VkInstance, VkSurfaceKHR*) -> void;
	auto window() const -> GLFWwindow*;
};
