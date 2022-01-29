#pragma once

#include "sge_engine/sge_engine_api.h"
#include "sge_utils/math/Box.h"
#include "sge_utils/math/transform.h"
#include "BulletHelper.h"

#include <memory>
#include <vector>

SGE_NO_WARN_BEGIN
#include <BulletCollision/CollisionDispatch/btGhostObject.h>
#include <BulletCollision/CollisionShapes/btTriangleMesh.h>
#include <BulletCollision/CollisionShapes/btTriangleMeshShape.h>
#include <btBulletDynamicsCommon.h>
SGE_NO_WARN_END

namespace sge {

/// CollsionShapeDesc
/// @brief This structures stores the information about how to create a single collision shape.
/// Rigid Bodies (or Collision Shapes) might accept multiple descriptions to create compound collision shapes.
struct SGE_ENGINE_API CollsionShapeDesc {
	enum Type : int {
		type_box,
		type_sphere,
		type_capsule,
		type_cylinder,
		type_cone,
		type_convexPoly,
		type_triangleMesh,
		type_infinitePlane,
	};

	CollsionShapeDesc() = default;

	static CollsionShapeDesc createBox(const vec3f& halfDiagonal, const transf3d& offset = transf3d());
	static CollsionShapeDesc createBox(const AABox3f& box);
	static CollsionShapeDesc createSphere(const float radius, const transf3d& offset = transf3d());
	static CollsionShapeDesc createCapsule(const float height, const float radius, const transf3d& offset = transf3d());
	static CollsionShapeDesc createCylinder(const vec3f& halfDiagonal, const transf3d& offset = transf3d());
	static CollsionShapeDesc createCylinder(float height, float radius, const transf3d& offset = transf3d());
	static CollsionShapeDesc createCylinderBottomAligned(float height, float radius, transf3d offset = transf3d());
	static CollsionShapeDesc createCone(const float height, const float radius, const transf3d& offset = transf3d());
	static CollsionShapeDesc createConvexPoly(std::vector<vec3f> verts, std::vector<int> indices);
	static CollsionShapeDesc createTriMesh(std::vector<vec3f> verts, std::vector<int> indices);

	/// @brief Create an infinite plane rigid body. The body is defined with the canonical plane formula:
	/// Ax + By + Cz + D = 0, where (A,B,C) are the normal of the plane and D is the constant of the plane.
	static CollsionShapeDesc createInfinitePlane(vec3f planeNormal, float planeConstant);

  public:
	Type type = type_box;
	transf3d offset;

	// Boxes.
	vec3f boxHalfDiagonal = vec3f(0.5f);
	// Spheres.
	float sphereRadius = 0.5f;
	// Capsules.
	float capsuleHeight = 0.5f;
	float capsuleRadius = 0.25f;
	// Cylinder.
	vec3f cylinderHalfDiagonal = vec3f(0.5f);
	// Cone.
	float coneHeight = 1.f;
	float coneRadius = 0.5f;
	// Convex or Triangle meshes.
	std::vector<vec3f> verticesConvexOrTriMesh;
	std::vector<int> indicesConvexOrTriMesh;
	// Infinite Plane.
	vec3f infinitePlaneNormal = vec3f::axis_y();
	float infinitePlaneConst = 0.f;
};

/// CollisionShape
/// Represents a collision shape for a rigid body.
/// Do not share it between multiple rigid bodies, as this is possible but not supported by our wrappers yet!
struct SGE_ENGINE_API CollisionShape {
	CollisionShape() = default;
	~CollisionShape()
	{
		destroy();
	}

	void create(const CollsionShapeDesc* desc, const int numDesc);
	void destroy()
	{
		m_triangleMeshes.clear();
		m_btShape.reset(nullptr);
		m_desc.clear();
	}

	btCollisionShape* getBulletShape()
	{
		return m_btShape.get();
	}
	const btCollisionShape* getBulletShape() const
	{
		return m_btShape.get();
	}

  private:
	std::vector<CollsionShapeDesc> m_desc;

	// The main shape used to be attached to the bullet rigid body.
	std::unique_ptr<btCollisionShape> m_btShape;

	// In case the collision shape is represented by concave triangle mesh, this object stores the actual triangles used by bullet physics.
	std::vector<std::unique_ptr<btTriangleMesh>> m_triangleMeshes;
};

} // namespace sge
