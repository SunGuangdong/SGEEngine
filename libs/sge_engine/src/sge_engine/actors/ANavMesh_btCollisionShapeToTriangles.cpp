#include "ANavMesh_btCollisionShapeToTriangles.h"
#include "sge_engine/physics/CollisionShape.h"
#include "sge_utils/containers/Range.h"

namespace sge {

void pushIndices(std::vector<int>& outIndices, int a, int b, int c)
{
	outIndices.push_back(a);
	outIndices.push_back(b);
	outIndices.push_back(c);
}

void btBoxShapeToTriangles(
    const btBoxShape* const box,
    const mat4f& transformNoScaling,
    std::vector<vec3f>& outVertices,
    std::vector<int>& outIndices)
{
	// Caution:
	// getHalfExtentsWithMargin obtains the extent with local scaling applied!
	const vec3f extents = fromBullet(box->getHalfExtentsWithMargin());

	/// The coordinates are explained with Right-Handeded coord system, Y up
	const vec3f v0nz = vec3f(-extents.x, -extents.y, -extents.z);
	const vec3f v1nz = vec3f(+extents.x, -extents.y, -extents.z);
	const vec3f v2nz = vec3f(+extents.x, +extents.y, -extents.z);
	const vec3f v3nz = vec3f(-extents.x, +extents.y, -extents.z);

	const vec3f v0pz = vec3f(-extents.x, -extents.y, +extents.z);
	const vec3f v1pz = vec3f(+extents.x, -extents.y, +extents.z);
	const vec3f v2pz = vec3f(+extents.x, +extents.y, +extents.z);
	const vec3f v3pz = vec3f(-extents.x, +extents.y, +extents.z);

	/// This value holds the starting index, in the index buffer, for the vertices we've hadded to describe the box
	/// shape.
	const int newIndicesStart = int(outVertices.size());

	outVertices.push_back(mat_mul_pos(transformNoScaling, v0nz));
	outVertices.push_back(mat_mul_pos(transformNoScaling, v1nz));
	outVertices.push_back(mat_mul_pos(transformNoScaling, v2nz));
	outVertices.push_back(mat_mul_pos(transformNoScaling, v3nz));
	outVertices.push_back(mat_mul_pos(transformNoScaling, v0pz));
	outVertices.push_back(mat_mul_pos(transformNoScaling, v1pz));
	outVertices.push_back(mat_mul_pos(transformNoScaling, v2pz));
	outVertices.push_back(mat_mul_pos(transformNoScaling, v3pz));

	const int i0nz = newIndicesStart + 0;
	const int i1nz = newIndicesStart + 1;
	const int i2nz = newIndicesStart + 2;
	const int i3nz = newIndicesStart + 3;

	const int i0pz = newIndicesStart + 4;
	const int i1pz = newIndicesStart + 5;
	const int i2pz = newIndicesStart + 6;
	const int i3pz = newIndicesStart + 7;


	// Face looking towards -Z           Face looking towards +Z
	// v2nz ---- v3nz              Y     v3pz ---- v2pz    Y
	// |        /  |               ^     |        / |      ^
	// |  2   /    |               |     |  2   /   |      |
	// |    /      |               |     |    /     |      |
	// |  /   1    |               |     |  /   1   |      |
	// |/          |               |     |/         |      |
	// v1nz ---- v0nz     X <-------     v0pz ---- v1pz    -------> X

	// -Z
	pushIndices(outIndices, i1nz, i0nz, i3nz);
	pushIndices(outIndices, i1nz, i3nz, i2nz);

	// +Z
	pushIndices(outIndices, i0pz, i1pz, i2pz);
	pushIndices(outIndices, i0pz, i2pz, i3pz);

	// Face looking towards -X           Face looking towards +X
	// v3nz ---- v3pz     Y              v2pz ---- v2nz            Y
	// |        /  |      ^              |        / |              ^
	// |  2   /    |      |              |  2   /   |              |
	// |    /      |      |              |    /     |              |
	// |  /   1    |      |              |  /   1   |              |
	// |/          |      |              |/         |              |
	// v0nz ---- v0pz     ------->Z     v1pz ---- v1nz    X <-------

	// -X
	pushIndices(outIndices, i0nz, i0pz, i3pz);
	pushIndices(outIndices, i0nz, i3pz, i3nz);

	// +X
	pushIndices(outIndices, i1pz, i1nz, i2nz);
	pushIndices(outIndices, i1pz, i2nz, i2pz);


	//   Face looking towards -Y        Face looking towards +Y
	//   v0pz ---- v1pz   Z             v3nz ---- v2nz     ------->X
	//   |        / |     ^              |        /  |     |
	//   |  2   /   |     |              |  2   /    |     |
	//   |    /     |     |              |    /      |     |
	//   |  /   1   |     |              |  /   1    |     |
	//   |/         |     |              |/          |     |
	//  v0nz ---- v1nz    -------> X    v3pz ---- v2pz     Z

	// -Y
	pushIndices(outIndices, i0nz, i1nz, i1pz);
	pushIndices(outIndices, i0nz, i1pz, i0pz);

	// +Y
	pushIndices(outIndices, i3pz, i2pz, i2nz);
	pushIndices(outIndices, i3pz, i2nz, i3nz);
}

/// @param numHorizontalSegments is the total number of segments (or vertices in a row) around the cylinder.
void btCylinderShapeToTriangles(
    const btCylinderShape* cylinderShape,
    const mat4f& transformNoScaling,
    std::vector<vec3f>& outVertices,
    std::vector<int>& outIndices,
    const int numHorizontalSegments = 8)
{
	// Caution:
	// getHalfExtentsWithMargin obtains the extent with local scaling applied!
	const vec3f extents = fromBullet(cylinderShape->getHalfExtentsWithMargin());

	const int newIndicesStart = int(outVertices.size());

	// Start by generating a cylider with it's geometric center located at 0,0,0,
	// with height 2 and radius 1:
	float kRadius = 1.f;
	float kHalfHeight = 1.f;

	// Coordinates on top of the circle, on xz plane.
	float x2 = kRadius;
	float z2 = 0;

	// Cylinder with height along Y axis at position.
	const float theta = 2.f * sgePi / numHorizontalSegments;
	const float c = cosf(theta);
	const float s = sinf(theta);

	// Generate the vertices in the following order:
	// v1-----v3----v5 ----v7        -----v1		|  ----------->x v0
	// |\     |\     |\     |       |\     |		|  |              .
	// | \ 2  | \    | \    |       | \    |		|  |            .
	// |  \   |  \   |  \   | . . . |  \   |		|  |           v2
	// |   \  |   \  |   \  |       |   \  |		|  |        .
	// |  1 \ |    \ |    \ |       |    \ |		|  |       .
	// |     \|     \|     \|       |     \|		|  z    v4
	// v0-----v2----v4 ----v6        -----v0        |  (Bottom view -camera is looking towards +Y)

	outVertices.reserve(outVertices.size() + numHorizontalSegments * 2 + 2);
	outIndices.reserve(outIndices.size() + numHorizontalSegments * 6 + numHorizontalSegments * 3 * 2);
	for (int i = 0; i < numHorizontalSegments; ++i) {
		const vec3f vBottom = mat_mul_pos(transformNoScaling, vec3f(x2, -kHalfHeight, z2) * extents);
		const vec3f vTop = mat_mul_pos(transformNoScaling, vec3f(x2, +kHalfHeight, z2) * extents);

		outVertices.push_back(vBottom);
		outVertices.push_back(vTop);

		// Next position... Think of this as a expanded matrix2x2 multiplication.
		const float x3 = x2;
		x2 = c * x2 - s * z2;
		z2 = s * x3 + c * z2;
	}

	// Add the indices forming the triangles around the cylinder.
	// the last quad of triangles, that connects the last column of vertices to the first one
	// if written manually.
	for (const int iSegment : RangeInt(numHorizontalSegments - 1)) {
		const int segment1stVertex = iSegment * 2 + newIndicesStart;

		outIndices.push_back(segment1stVertex + 0);
		outIndices.push_back(segment1stVertex + 2);
		outIndices.push_back(segment1stVertex + 1);

		outIndices.push_back(segment1stVertex + 1);
		outIndices.push_back(segment1stVertex + 2);
		outIndices.push_back(segment1stVertex + 3);
	}

	// Add the last triangles around the wall.
	{
		const int vn = (numHorizontalSegments - 1) * 2 + newIndicesStart;
		const int vn1 = vn + 1;
		const int v0 = newIndicesStart;
		const int v1 = v0 + 1;

		outIndices.push_back(v0);
		outIndices.push_back(vn1);
		outIndices.push_back(vn);

		outIndices.push_back(v0);
		outIndices.push_back(v1);
		outIndices.push_back(vn1);
	}

	// Add the cap vertices and triangles.
	{
		const int bottomCapVertexIdx = int(outVertices.size());
		outVertices.push_back(mat_mul_pos(transformNoScaling, vec3f(0.f, -kHalfHeight, 0.f) * extents));

		const int topCapVertexIdx = int(outVertices.size());
		outVertices.push_back(mat_mul_pos(transformNoScaling, vec3f(0.f, kHalfHeight, 0.f) * extents));

		// Bottom cap triangles.
		for (const int iSegment : RangeInt(numHorizontalSegments)) {
			const int segment1stVertex = iSegment * 2 + newIndicesStart;
			const int nextSegmentVertex = newIndicesStart + ((iSegment * 2 + 2) % (numHorizontalSegments * 2));
			outIndices.push_back(bottomCapVertexIdx);
			outIndices.push_back(nextSegmentVertex);
			outIndices.push_back(segment1stVertex);
		}

		// Top cap triangles.
		for (const int iSegment : RangeInt(numHorizontalSegments)) {
			const int segment1stVertex = iSegment * 2 + newIndicesStart + 1;
			const int nextSegmentVertex = newIndicesStart + ((iSegment * 2 + 2) % (numHorizontalSegments * 2)) + 1;
			outIndices.push_back(topCapVertexIdx);
			outIndices.push_back(segment1stVertex);
			outIndices.push_back(nextSegmentVertex);
		}
	}
}

/// Create an UV sphere representing the collsiion rigid body with triangles.
/// Non-uniform scaling is not supported.
void btSphereShapeToTriangles(
    const btSphereShape* btSphereShape,
    const mat4f& transformNoScaling,
    std::vector<vec3f>& outVertices,
    std::vector<int>& outIndices,
    const int numRings = 6,
    const int numHorizontalSegments = 6)
{
	outVertices.reserve(outVertices.size() + numRings * numHorizontalSegments + 2);
	outIndices.reserve(outIndices.size() + (numRings - 1) * numHorizontalSegments * 6 + 2 * numHorizontalSegments * 3);

	// Caution:
	// getHalfExtentsWithMargin obtains the extent with local scaling applied!
	const float radius = btSphereShape->getRadius(); // Yep, no non-uniform scalings...

	const float ringStepRad = sgePi / float(numRings + 1);
	const float sectorStepRad = sge2Pi / float(numHorizontalSegments);

	// Add the vertices that will represent the sphere.

	// Bottom vertex, for the bottom cap.
	const int bottomCapVertexIndex = int(outVertices.size());
	outVertices.push_back(mat_mul_pos(transformNoScaling, vec3f(0.f, -radius, 0.f)));

	// The vertices that will generate quads around the sphere, excluding
	// the cap vertices.
	const int bottomRing1stVertex = int(outVertices.size());
	for (int iRing = 0; iRing < numRings; ++iRing) {
		const float ringAngle = -pi() + float(iRing + 1) * ringStepRad;
		for (int iSegment = 0; iSegment < numHorizontalSegments; ++iSegment) {
			const float segmentAngle = iSegment * sectorStepRad;
			vec3f v;
			v.x = cosf(segmentAngle) * sinf(ringAngle) * radius;
			v.y = cosf(ringAngle) * radius;
			v.z = sinf(segmentAngle) * sinf(ringAngle) * radius;

			outVertices.push_back(mat_mul_pos(transformNoScaling, v));
		}
	}

	// Top vertex, for the top cap
	const int topCapVertexIndex = int(outVertices.size());
	outVertices.push_back(mat_mul_pos(transformNoScaling, vec3f(0.f, +radius, 0.f)));

	// Add the triangles that form the quads around the sphere.
	for (int iRing = 0; iRing < (numRings - 1); ++iRing) {
		for (int iSegment = 0; iSegment < numHorizontalSegments; ++iSegment) {
			//  i1rt------i0rt       Y
			//     |      /|         ^
			//     | 2  /  |         |
			//     |  /    |         |
			//     |/   1  |         .---------->X
			//  i1rb------i0rb      /
			//                     /
			//                    Z

			// A reminder that numHorizontalSegments is basically the number of vertices along a single ring.
			const int i0rb = bottomRing1stVertex + (iSegment + iRing * numHorizontalSegments);
			const int i0rt = bottomRing1stVertex + (iSegment + (iRing + 1) * numHorizontalSegments);
			const int i1rb =
			    bottomRing1stVertex + ((iSegment + 1) % numHorizontalSegments + iRing * numHorizontalSegments);
			const int i1rt =
			    bottomRing1stVertex + ((iSegment + 1) % numHorizontalSegments + (iRing + 1) * numHorizontalSegments);

			outIndices.push_back(i0rb);
			outIndices.push_back(i1rb);
			outIndices.push_back(i0rt);

			outIndices.push_back(i1rb);
			outIndices.push_back(i1rt);
			outIndices.push_back(i0rt);
		}
	}

	// Add the bottom cap triangles.
	{
		for (int iSegment = 0; iSegment < numHorizontalSegments; ++iSegment) {
			const int i0rb = bottomRing1stVertex + iSegment;
			const int i1rb = bottomRing1stVertex + (iSegment + 1) % numHorizontalSegments;

			outIndices.push_back(bottomCapVertexIndex);
			outIndices.push_back(i1rb);
			outIndices.push_back(i0rb);
		}
	}

	// Add the top cap triangles.
	{
		for (int iSegment = 0; iSegment < numHorizontalSegments; ++iSegment) {
			const int i0rt = bottomRing1stVertex + iSegment + (numRings - 1) * numHorizontalSegments;
			const int i1rt =
			    bottomRing1stVertex + (iSegment + 1) % numHorizontalSegments + (numRings - 1) * numHorizontalSegments;

			outIndices.push_back(topCapVertexIndex);
			outIndices.push_back(i1rt);
			outIndices.push_back(i0rt);
		}
	}
}

/// Caution: [CONVEX_HULLS_TRIANGLE_USER_DATA]:
/// Extracts the customly stored triangle mesh representation from the specified convexHullShape
bool btConvexHullShapeToTriangles(
    const btConvexHullShape* const convexHullShape,
    const mat4f& transformNoScaling,
    std::vector<vec3f>& outVertices,
    std::vector<int>& outIndices)
{
	// Caution [CONVEX_HULLS_TRIANGLE_USER_DATA]:
	// All convex hulls must have their triangle representation set as a user pointer!
	const vec3f localScaling = fromBullet(convexHullShape->getLocalScaling());
	sgeAssert(
	    localScaling.x >= 0.f && localScaling.y >= 0.f && localScaling.z > 0.f &&
	    "TODO handle negative scaling. The triangle winding must be flipped");

	CollsionShapeDesc* collisionShapeDesc = reinterpret_cast<CollsionShapeDesc*>(convexHullShape->getUserPointer());
	if (collisionShapeDesc == nullptr) {
		return false;
	}

	const int newIndicesStart = int(outVertices.size());

	// Copy the vertices.
	outVertices.reserve(outVertices.size() + collisionShapeDesc->verticesConvexOrTriMesh.size());
	for (const vec3f& vertex : collisionShapeDesc->verticesConvexOrTriMesh) {
		outVertices.push_back(mat_mul_pos(transformNoScaling, vertex * localScaling));
	}

	// Copy the indices and apply the index offset.
	outIndices.reserve(outIndices.size() + collisionShapeDesc->indicesConvexOrTriMesh.size());
	for (const int& index : collisionShapeDesc->indicesConvexOrTriMesh) {
		outIndices.push_back(index + newIndicesStart);
	}

	return true;
}

bool btBvhTriangleMeshShapeToTriangles(
    const btBvhTriangleMeshShape* bvhTriMeshShape,
    const mat4f& transformNoScaling,
    std::vector<vec3f>& outVertices,
    std::vector<int>& outIndices)
{
	const btStridingMeshInterface* const bulletMeshInterface = bvhTriMeshShape->getMeshInterface();

	if (bulletMeshInterface == nullptr) {
		return false;
	}

	auto processTriangle = [&](btVector3 triangleVertsScaled[3]) -> void {
		const int idxOffset = int(outVertices.size());

		outVertices.push_back(mat_mul_pos(transformNoScaling, fromBullet(triangleVertsScaled[0])));
		outVertices.push_back(mat_mul_pos(transformNoScaling, fromBullet(triangleVertsScaled[1])));
		outVertices.push_back(mat_mul_pos(transformNoScaling, fromBullet(triangleVertsScaled[2])));

		outIndices.push_back(idxOffset + 0);
		outIndices.push_back(idxOffset + 1);
		outIndices.push_back(idxOffset + 2);
	};

	const btVector3 meshScaling = bulletMeshInterface->getScaling();

	/// if the number of parts is big, the performance might drop due to the innerloop switch on indextype
	const int numSubParts = bulletMeshInterface->getNumSubParts();
	for (int part = 0; part < numSubParts; part++) {
		const unsigned char* vertexBase = nullptr;
		const unsigned char* indexBase = nullptr;
		int indexStride = 0;
		PHY_ScalarType type;
		PHY_ScalarType gfxindextype;
		int stride = 0;
		int numVerts = 0;
		int numTriangles = 0;
		btVector3 triangle[3];

		bulletMeshInterface->getLockedReadOnlyVertexIndexBase(
		    &vertexBase, numVerts, type, stride, &indexBase, indexStride, numTriangles, gfxindextype, part);

		switch (type) {
			case PHY_FLOAT: {
				float* graphicsbase;

				switch (gfxindextype) {
					case PHY_INTEGER: {
						for (int gfxindex = 0; gfxindex < numTriangles; gfxindex++) {
							unsigned int* tri_indices = (unsigned int*)(indexBase + gfxindex * indexStride);
							graphicsbase = (float*)(vertexBase + tri_indices[0] * stride);
							triangle[0].setValue(
							    graphicsbase[0] * meshScaling.getX(),
							    graphicsbase[1] * meshScaling.getY(),
							    graphicsbase[2] * meshScaling.getZ());
							graphicsbase = (float*)(vertexBase + tri_indices[1] * stride);
							triangle[1].setValue(
							    graphicsbase[0] * meshScaling.getX(),
							    graphicsbase[1] * meshScaling.getY(),
							    graphicsbase[2] * meshScaling.getZ());
							graphicsbase = (float*)(vertexBase + tri_indices[2] * stride);
							triangle[2].setValue(
							    graphicsbase[0] * meshScaling.getX(),
							    graphicsbase[1] * meshScaling.getY(),
							    graphicsbase[2] * meshScaling.getZ());
							processTriangle(triangle);
						}
						break;
					}
					case PHY_SHORT: {
						for (int gfxindex = 0; gfxindex < numTriangles; gfxindex++) {
							unsigned short int* tri_indices = (unsigned short int*)(indexBase + gfxindex * indexStride);
							graphicsbase = (float*)(vertexBase + tri_indices[0] * stride);
							triangle[0].setValue(
							    graphicsbase[0] * meshScaling.getX(),
							    graphicsbase[1] * meshScaling.getY(),
							    graphicsbase[2] * meshScaling.getZ());
							graphicsbase = (float*)(vertexBase + tri_indices[1] * stride);
							triangle[1].setValue(
							    graphicsbase[0] * meshScaling.getX(),
							    graphicsbase[1] * meshScaling.getY(),
							    graphicsbase[2] * meshScaling.getZ());
							graphicsbase = (float*)(vertexBase + tri_indices[2] * stride);
							triangle[2].setValue(
							    graphicsbase[0] * meshScaling.getX(),
							    graphicsbase[1] * meshScaling.getY(),
							    graphicsbase[2] * meshScaling.getZ());
							processTriangle(triangle);
						}
						break;
					}
					case PHY_UCHAR: {
						for (int gfxindex = 0; gfxindex < numTriangles; gfxindex++) {
							unsigned char* tri_indices = (unsigned char*)(indexBase + gfxindex * indexStride);
							graphicsbase = (float*)(vertexBase + tri_indices[0] * stride);
							triangle[0].setValue(
							    graphicsbase[0] * meshScaling.getX(),
							    graphicsbase[1] * meshScaling.getY(),
							    graphicsbase[2] * meshScaling.getZ());
							graphicsbase = (float*)(vertexBase + tri_indices[1] * stride);
							triangle[1].setValue(
							    graphicsbase[0] * meshScaling.getX(),
							    graphicsbase[1] * meshScaling.getY(),
							    graphicsbase[2] * meshScaling.getZ());
							graphicsbase = (float*)(vertexBase + tri_indices[2] * stride);
							triangle[2].setValue(
							    graphicsbase[0] * meshScaling.getX(),
							    graphicsbase[1] * meshScaling.getY(),
							    graphicsbase[2] * meshScaling.getZ());
							processTriangle(triangle);
						}
						break;
					}
					default:
						btAssert((gfxindextype == PHY_INTEGER) || (gfxindextype == PHY_SHORT));
				}
				break;
			}

			case PHY_DOUBLE: {
				double* graphicsbase = nullptr;

				switch (gfxindextype) {
					case PHY_INTEGER: {
						for (int gfxindex = 0; gfxindex < numTriangles; gfxindex++) {
							unsigned int* tri_indices = (unsigned int*)(indexBase + gfxindex * indexStride);
							graphicsbase = (double*)(vertexBase + tri_indices[0] * stride);
							triangle[0].setValue(
							    (btScalar)graphicsbase[0] * meshScaling.getX(),
							    (btScalar)graphicsbase[1] * meshScaling.getY(),
							    (btScalar)graphicsbase[2] * meshScaling.getZ());
							graphicsbase = (double*)(vertexBase + tri_indices[1] * stride);
							triangle[1].setValue(
							    (btScalar)graphicsbase[0] * meshScaling.getX(),
							    (btScalar)graphicsbase[1] * meshScaling.getY(),
							    (btScalar)graphicsbase[2] * meshScaling.getZ());
							graphicsbase = (double*)(vertexBase + tri_indices[2] * stride);
							triangle[2].setValue(
							    (btScalar)graphicsbase[0] * meshScaling.getX(),
							    (btScalar)graphicsbase[1] * meshScaling.getY(),
							    (btScalar)graphicsbase[2] * meshScaling.getZ());
							processTriangle(triangle);
						}
						break;
					}
					case PHY_SHORT: {
						for (int gfxindex = 0; gfxindex < numTriangles; gfxindex++) {
							unsigned short int* tri_indices = (unsigned short int*)(indexBase + gfxindex * indexStride);
							graphicsbase = (double*)(vertexBase + tri_indices[0] * stride);
							triangle[0].setValue(
							    (btScalar)graphicsbase[0] * meshScaling.getX(),
							    (btScalar)graphicsbase[1] * meshScaling.getY(),
							    (btScalar)graphicsbase[2] * meshScaling.getZ());
							graphicsbase = (double*)(vertexBase + tri_indices[1] * stride);
							triangle[1].setValue(
							    (btScalar)graphicsbase[0] * meshScaling.getX(),
							    (btScalar)graphicsbase[1] * meshScaling.getY(),
							    (btScalar)graphicsbase[2] * meshScaling.getZ());
							graphicsbase = (double*)(vertexBase + tri_indices[2] * stride);
							triangle[2].setValue(
							    (btScalar)graphicsbase[0] * meshScaling.getX(),
							    (btScalar)graphicsbase[1] * meshScaling.getY(),
							    (btScalar)graphicsbase[2] * meshScaling.getZ());
							processTriangle(triangle);
						}
						break;
					}
					case PHY_UCHAR: {
						for (int gfxindex = 0; gfxindex < numTriangles; gfxindex++) {
							unsigned char* tri_indices = (unsigned char*)(indexBase + gfxindex * indexStride);
							graphicsbase = (double*)(vertexBase + tri_indices[0] * stride);
							triangle[0].setValue(
							    (btScalar)graphicsbase[0] * meshScaling.getX(),
							    (btScalar)graphicsbase[1] * meshScaling.getY(),
							    (btScalar)graphicsbase[2] * meshScaling.getZ());
							graphicsbase = (double*)(vertexBase + tri_indices[1] * stride);
							triangle[1].setValue(
							    (btScalar)graphicsbase[0] * meshScaling.getX(),
							    (btScalar)graphicsbase[1] * meshScaling.getY(),
							    (btScalar)graphicsbase[2] * meshScaling.getZ());
							graphicsbase = (double*)(vertexBase + tri_indices[2] * stride);
							triangle[2].setValue(
							    (btScalar)graphicsbase[0] * meshScaling.getX(),
							    (btScalar)graphicsbase[1] * meshScaling.getY(),
							    (btScalar)graphicsbase[2] * meshScaling.getZ());
							processTriangle(triangle);
						}
						break;
					}
					default:
						sgeAssert((gfxindextype == PHY_INTEGER) || (gfxindextype == PHY_SHORT));
				}
				break;
			}
			default:
				sgeAssert((type == PHY_FLOAT) || (type == PHY_DOUBLE));
		}

		bulletMeshInterface->unLockReadOnlyVertexBase(part);
	}

	return true;
}

void bulletCollisionShapeToTriangles(
    const btCollisionShape* const collisionShape,
    const btTransform& parentTransform,
    std::vector<vec3f>& outVertices,
    std::vector<int>& outIndices)
{
	const btCompoundShape* const compoundShape =
	    btCollisionShapeCast<btCompoundShape>(collisionShape, COMPOUND_SHAPE_PROXYTYPE);
	const btBoxShape* const boxShape = btCollisionShapeCast<btBoxShape>(collisionShape, BOX_SHAPE_PROXYTYPE);
	const btSphereShape* const sphereShape =
	    btCollisionShapeCast<btSphereShape>(collisionShape, SPHERE_SHAPE_PROXYTYPE);
	const btCylinderShape* const cylinderShape =
	    btCollisionShapeCast<btCylinderShape>(collisionShape, CYLINDER_SHAPE_PROXYTYPE);
	const btConvexHullShape* const convexHullShape =
	    btCollisionShapeCast<btConvexHullShape>(collisionShape, CONVEX_HULL_SHAPE_PROXYTYPE);
	const btBvhTriangleMeshShape* const bvhTriMeshShape =
	    btCollisionShapeCast<btBvhTriangleMeshShape>(collisionShape, TRIANGLE_MESH_SHAPE_PROXYTYPE);

	if (compoundShape != nullptr) {
		const int numChilds = compoundShape->getNumChildShapes();
		for (int iChild = 0; iChild < numChilds; ++iChild) {
			const btCollisionShape* childShape = compoundShape->getChildShape(iChild);
			const btTransform childTransform = compoundShape->getChildTransform(iChild);
			const btTransform childFullTransform = parentTransform * childTransform;

			bulletCollisionShapeToTriangles(childShape, childFullTransform, outVertices, outIndices);
		}
	}
	else if (boxShape) {
		btBoxShapeToTriangles(boxShape, fromBullet(parentTransform).toMatrix(), outVertices, outIndices);
	}
	else if (sphereShape) {
		btSphereShapeToTriangles(sphereShape, fromBullet(parentTransform).toMatrix(), outVertices, outIndices);
	}
	else if (cylinderShape) {
		btCylinderShapeToTriangles(cylinderShape, fromBullet(parentTransform).toMatrix(), outVertices, outIndices);
	}
	else if (convexHullShape) {
		[[maybe_unused]] bool succeeded = btConvexHullShapeToTriangles(
		    convexHullShape, fromBullet(parentTransform).toMatrix(), outVertices, outIndices);
		sgeAssert(succeeded);
	}
	else if (bvhTriMeshShape) {
		[[maybe_unused]] bool succeeded = btBvhTriangleMeshShapeToTriangles(
		    bvhTriMeshShape, fromBullet(parentTransform).toMatrix(), outVertices, outIndices);
		sgeAssert(succeeded);
	}
	else {
		sgeAssert(false && "Unimplemented collision shape. The shape will be skipped");
	}

	sgeAssert(outIndices.size() % 3 == 0 && "The function should output triangle lists.");
}

} // namespace sge
