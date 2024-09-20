#version 450

layout (location = 0) in vec2 inUV;

layout (binding = 0) uniform sampler2D image;

layout (location = 0) out vec4 fragColor;

void main() {
	// ivec2 textureDimensions = textureSize(image, 0);
	fragColor = vec4(texture(image, inUV).xyz, 1.0);
}
