
// structs representing items from cpu memory (via ssbo or uniform or other buffer)
// will use vec4 instead of vec3, as vec3 seems induces lots of weird alignment cases
// purely shader side elements will still use vec3s

struct Model { // cpu side
	mat4 modelMatrix;
};

struct Material { // cpu side
	vec4 albedo;
	uint materialType;
};

struct Triangle { // cpu side
	vec4 v0;
	vec4 v1;
	vec4 v2;
	uint materialIndex;
	uint modelIndex;
};

struct Sphere { // cpu side
	vec4 center;
	float radius;
	uint materialIndex;
	uint modelIndex;
};

struct Light {
	float area;
	uint triangleIndex;
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
	float u;
	float v;
};

struct AABB {
	float minX; float maxX;
	float minY; float maxY;
	float minZ; float maxZ;
};

struct MortonPrimitive {
	uint code;
	uint primitiveIndex;
	uint primitiveType;
};

#define INVALID_HLBVHNODE_INDEX 0

struct HLBVHNode {
	AABB aabb;
	uint leftIndex;
	uint rightIndex;
	uint primitiveIndex;
	uint primitiveType;
};

struct HLBVHAABBConstructionInfo {
	uint parent;
	int visitationCount;
};

#define LIGHT_MATERIAL 0
#define DIFFUSE_MATERIAL 1
#define METALLIC_MATERIAL 2
#define DIELECTRIC_MATERIAL 3

#define SPHERE_PRIMITIVE 0
#define TRIANGLE_PRIMITIVE 1
