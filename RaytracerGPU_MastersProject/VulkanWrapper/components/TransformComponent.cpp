
#include "TransformComponent.hpp"

// Matrix corresponds to translate * Ry * Rx * Rz * scale transformation
// Rotation convention uses Tait-Bryan Angles with axis order Y, X, Z
// R = Y X Z
// To interpret as an extrinsic rotation: read from right to left
// To interpret as an intrinsic rotation: read from left to right
auto TransformComponent::mat4() const -> glm::mat4 {
	/*
		[ a b c tx
		  d e f ty
		  g h i tz
		  0 0 0 1  ]
			// 3d affine transformation via homogeneous coordinates
	*/
	// y x z Tait-Bryan Angle (Euler Angle) https://en.wikipedia.org/wiki/Euler_angles#Rotation_matrix
	const float c3 = glm::cos(rotation.z);
	const float s3 = glm::sin(rotation.z);
	const float c2 = glm::cos(rotation.x);
	const float s2 = glm::sin(rotation.x);
	const float c1 = glm::cos(rotation.y);
	const float s1 = glm::sin(rotation.y);
	return glm::mat4{
		{ // col 1
			scale.x * (c1 * c3 + s1 * s2 * s3),		// a
			scale.x * (c2 * s3),						// d
			scale.x * (c1 * s2 * s3 - c3 * s1),		// g
			0.0f									// 0
		},
		{
			scale.y * (c3 * s1 * s2 - c1 * s3),		// b
			scale.y * (c2 * c3),					// e
			scale.y * (c1 * c3 * s2 + s1 * s3),		// h
			0.0f									// 0
		},
		{
			scale.z * (c2 * s1),					// c
			scale.z * (-s2),						// f
			scale.z * (c1 * c2),					// i
			0.0f									// 0
		},
		{ translation.x, translation.y, translation.z, 1.0f} // tx, ty, tz, 1
	};
}

/*
	modelMatrix ^-1 ^T = (TRS)^-1^T. T = translation, R = rotation, S = scale
	normals are unaffected by translations
	= (RS)^-1^T
	(AB)^-1 = B^-1 A^-1
	= (S^-1 R^-1)^T
	(AB)^T = B^T A^T
	= (R^-1)^T (S^-1)^T
	Rotation matrix: R^-1 = R^T. S^-1 is a diagonal matrix so D^T = D
	= (R^T)^T S^-1
	A^T^T = A
	= R(S^-1) 3x3
*/
auto TransformComponent::normalMatrix() const -> glm::mat3 {
	const float c3 = glm::cos(rotation.z);
	const float s3 = glm::sin(rotation.z);
	const float c2 = glm::cos(rotation.x);
	const float s2 = glm::sin(rotation.x);
	const float c1 = glm::cos(rotation.y);
	const float s1 = glm::sin(rotation.y);
	const glm::vec3 invScale = 1.0f / scale;
	return glm::mat3{
		{	// see mat4() for description of rotation and scale
			invScale.x * (c1 * c3 + s1 * s2 * s3),
			invScale.x * (c2 * s3),
			invScale.x * (c1 * s2 * s3 - c3 * s1),
		},
		{
			invScale.y * (c3 * s1 * s2 - c1 * s3),
			invScale.y * (c2 * c3),
			invScale.y * (c1 * c3 * s2 + s1 * s3),
		},
		{
			invScale.z * (c2 * s1),
			invScale.z * (-s2),
			invScale.z * (c1 * c2),
		}
	};
}
