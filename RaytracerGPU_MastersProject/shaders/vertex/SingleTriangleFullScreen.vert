#version 450
#extension GL_KHR_vulkan_glsl: enable
// the extension above doesn't seem to do anything (https://gamedev.stackexchange.com/questions/191231/get-a-warning-about-extension-when-compile-glsl-code-with-glslc-compiler-shipped)
// but the linter doesn't like it if it isn't there. so keeping for now.

layout (location = 0) out vec2 outUV;

out gl_PerVertex {
	vec4 gl_Position;
};

void main() { // https://www.saschawillems.de/blog/2016/08/13/vulkan-tutorial-on-rendering-a-fullscreen-quad-without-buffers/
	outUV = vec2((gl_VertexIndex << 1) & 2, gl_VertexIndex & 2);
	gl_Position = vec4(outUV * 2.0f - 1.0f, 0.0f, 1.0f);
	// basically, this using the vertex indecies (0, 1, & 2) to generate three verticies to form a triangle which
	// entirely contains the viewport rectangle.
}
