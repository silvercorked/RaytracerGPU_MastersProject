#version 450

layout (location = 0) in vec2 inUV;

layout (binding = 0) uniform UBO {
	float a;	
} ubo;

layout (binding = 1) uniform sampler2D image;

layout (location = 0) out vec4 fragColor;

void main() {
	// ivec2 textureDimensions = textureSize(image, 0);
	fragColor = texture(image, inUV);
}
