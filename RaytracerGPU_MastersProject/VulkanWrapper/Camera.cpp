
#define GLM_FORCE_RADIANS					// functions expect radians, not degrees
#define GLM_FORCE_DEPTH_ZERO_TO_ONE			// Depth buffer values will range from 0 to 1, not -1 to 1
#include <glm/glm.hpp>

#include <cassert>

module VulkanWrap:Camera;

/*
	Transformation order:
	1) Model transform (from object space to world space)
	2) Camera transform (to camera space)
	3) Projection transform (to canonical view volume)
	4) Viewport transform (to viewport)

	- model defined in object space
	- Transform Component in GameObject.hpp performs step 1 (model transformation matrix)
	- camera applies projection transformation
	- vulkan handles the viewport transform (i think)
*/

/*
	Ray tracing is pixel order rendering
	- takes a long longer because influence of all other objects must be taken into account

	Object order rendering (this project)
	- is faster
	- just draws objects in a particular order
	- requires matrix transformation

	Vulkan's canonical viewing volume:
	- x(-1, 1), y(-1, 1), z(0, 1)

	Orthographic projection:
	- dimensions and locations fixed
	- axis-aligned box
	- defined by 6 bounding planes (or 2 vertexes)
		 c011________c111
		   /|          /|
		c001________c101|
		|	|		|   |
		|   c010____|___c110
		|  /		|  /
		c000________c100
		take c000 to be (left, bottom, near) and c111 to be (right, top, far)
		to create matrix, need to transform from orthographic view volume to Vulkan's canonical view volume:
		- 1) translate center of plane c000,c100,c001,c101 to origin
			C = (right + left / 2, bottom + top / 2, near)
			[[ 1 0 0 -Cx ]
			 [ 0 1 0 -Cy ]
			 [ 0 0 1 -Cz ]
			 [ 0 0 0  1  ]]
		- 2) scale to size of canonical volume
			[[ 2/Sx    0    0 0 ] // 2 / Sx because x is over a range of 2 (1--1 = 2)
			 [    0 2/Sy    0 0 ] // Sy = bottom - top. * 1/Sy results in a 0-1 range
			 [    0    0 1/Sz 0 ] // 1 / Sz because z is only over a range of 1 (1 - 0 = 1)
			 [    0    0    0 1 ]]
		- 3) put them together (and simplify) for the orthographic projection matrix
			[[ 2/(r-l)       0       0 -(r+l)/(r-l) ]
			 [       0 2/(b-t)       0 -(b+t)/(b-t) ]
			 [       0       0 1/(f-n)     -n/(f-n) ]
			 [       0       0       0            1  ]]
		Different from other orthographic projection matricies because this specifically maps to Vulkan's canonical view volume
		- different shape than opengl
		- right handed coordinate system
		- looking down +z

	Perspective matrix:
	- square frustum
	- map to orthographic view volume
											 |
								   . target  |
			   ys 	               |y        | far plane (no rendering past here
	viewer .___|___________________|_________|
			 n |          z                  |
			   viewing plane
		ys / n = y / z by similar triangles
		ys = n * y / z
		// same idea in x direction
		want matrix such that
		[[?]][x y z 1] = [nx/z ny/z z 1]
		no 4x4 matrix can cause z division in result matrix, but homogeneous coordinate fix that
	- [x y z 1], 1 was for translation, but now define as w
	- the homogenous vector [x y z w] <=> (corresponds w/) [x/w y/w z/w]
		- [1 2 3 1] = [10 20 30 10] = [2 4 6 2] = [1 2 3]
		- this w division is automatically occuring on the gl_Position output variable of the vertex shader
		[[?1]][x y z 1] = [nx ny ?2 z] => feasible. ?2 = z^2 because after divide by w (z) z^2/z = z
		[[ n 0 0  0  ]  // times x and y by n
		 [ 0 n 0  0  ]
		 [ 0 0 m1 m2 ]  // can only use m1 and m2 to cause z^2
		 [ 0 0 1  0  ]] // move z to w position
		 m1*z + m2 = z^2, quadratic so 2 solutions (at most). so want solutions when z = n and z = f
			- this means z vals on near and far plane will be unchanged, but all other z values will be warped non-linearly
			- results in 2 equations:
			- m1 * n + m2 = n^2
			- m1 * f + m2 = f^2
			- solving the system: m1 = f + n, m2 = -f * n
		[[ n 0 0    0  ]
		 [ 0 n 0    0  ]
		 [ 0 0 f+n -f*n]
		 [ 0 0 1    0  ]]
		 - result: wanted linear behavior over z, but best that could be done was quadratic.
			- interestingly, this does come with the benefit of higher precision for more of the region compared to
				a linear relationship, which will help avoid z-fighting (z-stitching) which occurs when two objects
				swap between appearing and not appearing due to floating point precision errors
	Perspective Project Matrix:
	- map to vulkan canonical volume
	- apply perspective (from perspective view volume to orthographic view volume), then apply orthographic
	[[ 2n/(r-l) 0        -(r+l)/(r-l) 0         ]     [[ 2/(r-l) 0       0       -(r+l)/(r-l) ] [[ n 0 0    0  ]
	 [ 0        2n/(b-t) -(b+t)/(b-t) 0         ]  =   [ 0       2/(b-t) 0       -(b+t)/(b-t) ]  [ 0 n 0    0  ]
	 [ 0        0         f/(f-n)    -f*n/(f-n) ]      [ 0       0       1/(f-n) -n/(f-n)     ]  [ 0 0 f+n -fn ]
	 [ 0        0         1           0         ]]     [ 0       0       0        1           ]] [ 0 0 1    0  ]]
	- if we assume frustum is centered on z-axis:
		- r = -l, t = -b
		=> r + l = 0, b + t = 0
		=> r - l = 2r, b - t = 2b
	[[ n/r 0   0        0         ] // this allows several simplifications
	 [ 0   n/b 0        0         ]
	 [ 0   0   f/(f-n) -f*n/(f-n) ]
	 [ 0   0   1        0         ]]
	- it is common to use a vertical field of view rather than bottom and right plane values
	- aspect ratio (w/h) also involved
		- b = n tan(theta / 2), r = n * w/h tan(theta / 2)
	- finally, after simplfication
	[[ 1/((w/h)tan(theta/2)) 0                0        0         ]
	 [ 0                     1/(tan(theta/2)) 0        0         ]
	 [ 0                     0                f/(f-n) -f*n/(f-n) ]
	 [ 0                     0                1        0         ]] // final form of perspective projection matrix for vulkan
*/

auto Camera::setOrthographicProjection(float left, float right, float top, float bottom, float near, float far) -> void {
	this->projectionMatrix = glm::mat4{ 1.0f };
	this->projectionMatrix[0][0] = 2.0f / (right - left);
	this->projectionMatrix[1][1] = 2.0f / (bottom - top);
	this->projectionMatrix[2][2] = 1.0f / (far - near);
	this->projectionMatrix[3][0] = -(right + left) / (right - left);
	this->projectionMatrix[3][1] = -(bottom + top) / (bottom - top);
	this->projectionMatrix[3][2] = -near / (far - near);
}
auto Camera::setPerspectiveProjection(float fovy, float aspect, float near, float far) -> void {
	assert(glm::abs(aspect - std::numeric_limits<float>::epsilon()) > 0.0f);
	const float tanHalfFovY = tan(fovy / 2.0f);
	this->projectionMatrix = glm::mat4{ 0.0f };
	this->projectionMatrix[0][0] = 1.0f / (aspect * tanHalfFovY);
	this->projectionMatrix[1][1] = 1.0f / (tanHalfFovY);
	this->projectionMatrix[2][2] = far / (far - near);
	this->projectionMatrix[2][3] = 1.0f;
	this->projectionMatrix[3][2] = -(far * near) / (far - near);
}
/*
	[[ u.x u.y u.z 0 ]^-1  [[ 1 0 0 -p.x ]
		[ v.x v.y v.z 0 ]      [ 0 1 0 -p.y ]
		[ w.x w.y w.z 0 ]      [ 0 0 1 -p.z ]
		[ 0   0   0   1 ]]     [ 0 0 0  1   ]] // rotation * translation = camera transformation
		// inverse rotation because rotating from that angle back to canonical direction
		// transform is negative, because camera position is in world space, and we're moving it back to the origin
*/
auto Camera::setViewDirection(glm::vec3 position, glm::vec3 dir, glm::vec3 up) -> void {
	assert((dir != glm::vec3{ 0 } && "Camera.setViewDirection: Invalid direction. No direction given by zero vector."));
	const glm::vec3 w{ glm::normalize(dir) };
	const glm::vec3 u{ glm::normalize(glm::cross(w, up)) };
	const glm::vec3 v{ glm::cross(w, u) }; // construct orthonormal basis (unit length and orthogonal to eachother

	this->viewMatrix = glm::mat4{ 1.0f };
	this->viewMatrix[0][0] = u.x;
	this->viewMatrix[1][0] = u.y;
	this->viewMatrix[2][0] = u.z;
	this->viewMatrix[0][1] = v.x;
	this->viewMatrix[1][1] = v.y;
	this->viewMatrix[2][1] = v.z;
	this->viewMatrix[0][2] = w.x;
	this->viewMatrix[1][2] = w.y;
	this->viewMatrix[2][2] = w.z;
	this->viewMatrix[3][0] = -glm::dot(u, position);
	this->viewMatrix[3][1] = -glm::dot(v, position);
	this->viewMatrix[3][2] = -glm::dot(w, position);

	this->inverseViewMatrix = glm::mat4{ 1.0f };
	this->inverseViewMatrix[0][0] = u.x;
	this->inverseViewMatrix[0][1] = u.y;
	this->inverseViewMatrix[0][2] = u.z;
	this->inverseViewMatrix[1][0] = v.x;
	this->inverseViewMatrix[1][1] = v.y;
	this->inverseViewMatrix[1][2] = v.z;
	this->inverseViewMatrix[2][0] = w.x;
	this->inverseViewMatrix[2][1] = w.y;
	this->inverseViewMatrix[2][2] = w.z;
	this->inverseViewMatrix[3][0] = position.x;
	this->inverseViewMatrix[3][1] = position.y;
	this->inverseViewMatrix[3][2] = position.z;
}
auto Camera::setViewTarget(glm::vec3 position, glm::vec3 target, glm::vec3 up) -> void {
	this->setViewDirection(position, target - position, up);
}
auto Camera::setViewYXZ(glm::vec3 position, glm::vec3 rotation) -> void {
	const float c3 = glm::cos(rotation.z); // same idea as in GameObject's Transform Component used for rotation
	const float s3 = glm::sin(rotation.z); // except inverted this time
	const float c2 = glm::cos(rotation.x); // could negate all these rotations
	const float s2 = glm::sin(rotation.x); // but inverse of rotation matrix is also just transpose of rotation matrix
	const float c1 = glm::cos(rotation.y); // so no need, and can just set below in the correct way
	const float s1 = glm::sin(rotation.y);
	const glm::vec3 u{ (c1 * c3 + s1 * s2 * s3), (c2 * s3), (c1 * s2 * s3 - c3 * s1) };
	const glm::vec3 v{ (c3 * s1 * s2 - c1 * s3), (c2 * c3), (c1 * c3 * s2 + s1 * s3) };
	const glm::vec3 w{ (c2 * s1), (-s2), (c1 * c2) };
	this->viewMatrix = glm::mat4{ 1.0f };
	this->viewMatrix[0][0] = u.x;
	this->viewMatrix[1][0] = u.y;
	this->viewMatrix[2][0] = u.z;
	this->viewMatrix[0][1] = v.x;
	this->viewMatrix[1][1] = v.y;
	this->viewMatrix[2][1] = v.z;
	this->viewMatrix[0][2] = w.x;
	this->viewMatrix[1][2] = w.y;
	this->viewMatrix[2][2] = w.z;
	this->viewMatrix[3][0] = -glm::dot(u, position);
	this->viewMatrix[3][1] = -glm::dot(v, position);
	this->viewMatrix[3][2] = -glm::dot(w, position);

	this->inverseViewMatrix = glm::mat4{ 1.0f };
	this->inverseViewMatrix[0][0] = u.x;
	this->inverseViewMatrix[0][1] = u.y;
	this->inverseViewMatrix[0][2] = u.z;
	this->inverseViewMatrix[1][0] = v.x;
	this->inverseViewMatrix[1][1] = v.y;
	this->inverseViewMatrix[1][2] = v.z;
	this->inverseViewMatrix[2][0] = w.x;
	this->inverseViewMatrix[2][1] = w.y;
	this->inverseViewMatrix[2][2] = w.z;
	this->inverseViewMatrix[3][0] = position.x;
	this->inverseViewMatrix[3][1] = position.y;
	this->inverseViewMatrix[3][2] = position.z;
}
