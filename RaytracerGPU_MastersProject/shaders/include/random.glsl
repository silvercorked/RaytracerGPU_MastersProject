
// from https://github.com/grigoryoskin/vulkan-compute-ray-tracing/blob/master/resources/shaders/source/include/random.glsl

/*
	If included, including file must contain a uniform buffer object called ubo with a randomState
*/

const float pi = 3.1415926535897932385;

uint rngState = (600 * gl_GlobalInvocationID.x + gl_GlobalInvocationID.y) * (ubo.randomState + 1);

// random implementation follows implementation of pcg32i_random_t, with inc = 1
uint stepRNG(uint rngState) {
	return rngState * 747796405 + 1;
}

// pcg_output_rxs_m_xs_32_32 converted to a [0, 1] range
float stepAndOutputRNGFloat(inout uint rngState) {
	rngState = stepRNG(rngState);
	uint word = ((rngState >> ((rngState >> 28) + 4)) ^ rngState) * 277803737;
	word = (word >> 22) ^ word;
	return float(word) / 4294967295.0f;
}

float random() {
	return stepAndOutputRNGFloat(rngState);
}

float random(float min, float max) {
	return min + (max - min) * random();
}

vec3 randomInUnitSphere() {
	float rho = random();
	float theta = random(0, 2 * pi);
	float phi = random(0, pi);
	return vec3(rho * sin(phi) * cos(theta), rho * sin(phi) * sin(theta), rho * cos(phi));
}
vec3 randomInHemisphere(vec3 normal) {
	vec3 inUnitSphere = randomInUnitSphere();
	if (dot(inUnitSphere, normal) > 0.0) // less than 90 angle between the 2
		return inUnitSphere;
	else
		return -inUnitSphere;
}
vec3 randomCosineDirection() {
	float r1 = random();
	float r2 = random();
	float z = sqrt(1 - r2);
	float phi = 2 * pi * r1;
	float x = cos(phi) * sqrt(r2);
	float y = sin(phi) * sqrt(r2);
	return vec3(x, y, z);
}
vec3 randomInUnitDisk() {
	float theta = random(0, 2 * pi);
	float r = random(0, 1);
	return vec3(r * cos(theta), r * sin(theta), 0);
}
vec3 randomUnitVector() {
	return normalize(randomInUnitSphere());
}
