#version 450

layout (location = 0) in vec2 inUV;

layout(set = 0, binding = 0) uniform GlobalUbo {
	uint raysPerPixel;
} ubo;

layout (binding = 1) uniform sampler2D image;

layout (location = 0) out vec4 fragColor;

void main() {
	vec3 uncorrected = texture(image, inUV).xyz;
	vec3 corrected = clamp(
		sqrt(uncorrected / float(ubo.raysPerPixel))
		, 0.0, 1.0
	); // finishing gamma correction for gamma=2

	fragColor = vec4(corrected, 1.0);
}
