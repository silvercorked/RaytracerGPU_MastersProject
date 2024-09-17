
#include "Window.hpp"

#include <stdexcept>

Window::Window(int w, int h, std::string name) :
	_width(w),
	_height(h),
	_windowName(name),
	_framebufferResized(false)
{
	this->initWindow();
}
Window::~Window() {
	glfwDestroyWindow(_window);
	glfwTerminate();
}
auto Window::shouldClose() -> bool {
	return glfwWindowShouldClose(this->_window);
}
auto Window::getExtent() -> VkExtent2D {
	return { static_cast<uint32_t>(this->_width), static_cast<uint32_t>(this->_height) };
}
auto Window::wasWindowResized() -> bool {
	return this->_framebufferResized;
}
auto Window::resetWindowResizeFlag() -> void {
	this->_framebufferResized = false;
}
auto Window::createWindowSurface(VkInstance instance, VkSurfaceKHR* surface) -> void {
	if (glfwCreateWindowSurface(instance, _window, nullptr, surface) != VK_SUCCESS) {
		throw std::runtime_error("failed to create window surface");
	}
}
auto Window::window() const -> GLFWwindow* {
	return this->_window;
}
auto Window::frambufferResizeCallback(GLFWwindow* currWindow, int width, int height) -> void {
	auto w = reinterpret_cast<Window*>(glfwGetWindowUserPointer(currWindow));
	w->_framebufferResized = true;
	w->_width = width;
	w->_height = height;
}

auto Window::initWindow() -> void {
	glfwInit();
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);	// disable opengl context, since using vulkan
	glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);		// will handle resizing manually
	this->_window = glfwCreateWindow(
		this->_width,
		this->_height,
		this->_windowName.c_str(),
		nullptr,									// windowed mode
		nullptr										// opengl context, so unused
	);
	glfwSetWindowUserPointer(this->_window, this);
	glfwSetFramebufferSizeCallback(this->_window, Window::frambufferResizeCallback);
}