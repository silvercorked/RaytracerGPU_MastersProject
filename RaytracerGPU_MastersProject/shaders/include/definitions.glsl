
struct Model {
	mat4 modelMatrix;
};

struct Material {
	uint materialType;
	vec3 albedo;
};

struct Triangle {
	vec3 v0;
	vec3 v1;
	vec3 v2;
	uint materialIndex;
	uint modelIndex;
};

struct Sphere {
	vec3 center;
	float radius;
	uint materialIndex;
	uint modelIndex;
};

struct Light {
	uint triangleIndex;
	float area;
};

struct Ray {
	vec3 origin;
	vec3 direction;
};

struct HitRecord {
	vec3 p;
	vec3 normal;
	uint materialIndex;
	float t;
	int backFaceInt; // bool as int
	//float scatterPdf;
	//float samplePdf;
	vec3 u;
	vec3 v;
};

#define LIGHT_MATERIAL 0
#define DIFFUSE_MATERIAL 1
#define METALLIC_MATERIAL 2
#define DIELECTRIC_MATERIAL 3
