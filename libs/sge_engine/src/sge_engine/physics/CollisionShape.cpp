#include "CollisionShape.h"
#include "BulletHelper.h"

#include "sge_core/typelib/typeLib.h"

namespace sge {
// clang-format off
ReflAddTypeId(CollsionShapeDesc::Type,        21'02'28'0003);
ReflAddTypeId(CollsionShapeDesc,              21'02'28'0004);
ReflAddTypeId(std::vector<CollsionShapeDesc>, 21'02'28'0005);
ReflBlock()
{
	ReflAddType(CollsionShapeDesc::Type)
		ReflEnumVal(CollsionShapeDesc::type_box, "Box")
		ReflEnumVal(CollsionShapeDesc::type_sphere, "Sphere")
		ReflEnumVal(CollsionShapeDesc::type_capsule, "Capsule")
		ReflEnumVal(CollsionShapeDesc::type_cylinder, "Cylinder")
		ReflEnumVal(CollsionShapeDesc::type_cone, "Cone")
		ReflEnumVal(CollsionShapeDesc::type_convexPoly, "Convex")
		ReflEnumVal(CollsionShapeDesc::type_triangleMesh, "Triangle Mesh")
		ReflEnumVal(CollsionShapeDesc::type_infinitePlane, "Plane")
	;

	ReflAddType(CollsionShapeDesc)
		ReflMember(CollsionShapeDesc, type)
		ReflMember(CollsionShapeDesc, offset)
		ReflMember(CollsionShapeDesc, boxHalfDiagonal).uiRange(0.f, 1000000.f, 0.1f)
		ReflMember(CollsionShapeDesc, sphereRadius).uiRange(0.f, 1000000.f, 0.1f)
		ReflMember(CollsionShapeDesc, capsuleHeight).uiRange(0.f, 1000000.f, 0.1f)
		ReflMember(CollsionShapeDesc, capsuleRadius).uiRange(0.f, 1000000.f, 0.1f)
		ReflMember(CollsionShapeDesc, cylinderHalfDiagonal).uiRange(0.f, 1000000.f, 0.1f)
		ReflMember(CollsionShapeDesc, coneHeight).uiRange(0.f, 1000000.f, 0.1f)
		ReflMember(CollsionShapeDesc, coneRadius).uiRange(0.f, 1000000.f, 0.1f)
		ReflMember(CollsionShapeDesc, infinitePlaneNormal)
		ReflMember(CollsionShapeDesc, infinitePlaneConst)
	;

	ReflAddType(std::vector<CollsionShapeDesc>);
}
// clang-format on

//-------------------------------------------------------------------------
// CollsionShapeDesc
//-------------------------------------------------------------------------
CollsionShapeDesc CollsionShapeDesc::createBox(const vec3f& halfDiagonal, const transf3d& offset)
{
	CollsionShapeDesc r;
	r.type = type_box;
	r.boxHalfDiagonal = halfDiagonal * offset.s;
	r.offset = transf3d(offset.p, offset.r, vec3f(1.f));

	return r;
}

CollsionShapeDesc CollsionShapeDesc::createBox(const AABox3f& box)
{
	sgeAssert(box.IsEmpty() == false);

	CollsionShapeDesc r;
	r.type = type_box;
	r.boxHalfDiagonal = box.halfDiagonal();
	r.offset = transf3d(box.center());

	return r;
}

CollsionShapeDesc CollsionShapeDesc::createSphere(const float radius, const transf3d& offset)
{
	CollsionShapeDesc r;
	r.type = type_sphere;
	r.sphereRadius = radius * offset.s.hsum() / 3.f;
	r.offset = transf3d(offset.p, offset.r, vec3f(1.f));

	return r;
}

CollsionShapeDesc CollsionShapeDesc::createCapsule(const float height, const float radius, const transf3d& offset)
{
	CollsionShapeDesc r;
	r.type = type_capsule;
	r.capsuleHeight = height * offset.s.y;
	r.capsuleRadius = radius;
	r.offset = transf3d(offset.p, offset.r, vec3f(1.f));

	return r;
}

CollsionShapeDesc CollsionShapeDesc::createCylinder(const vec3f& halfDiagonal, const transf3d& offset)
{
	CollsionShapeDesc r;
	r.type = type_cylinder;
	r.cylinderHalfDiagonal = halfDiagonal * offset.s;
	r.offset = transf3d(offset.p, offset.r, vec3f(1.f));

	return r;
}

CollsionShapeDesc CollsionShapeDesc::createCylinder(float height, float radius, const transf3d& offset)
{
	vec3f halfDiagonal = vec3f(radius, height * 0.5f, radius);
	return createCylinder(halfDiagonal, offset);
}

CollsionShapeDesc CollsionShapeDesc::createCylinderBottomAligned(float height, float radius, transf3d offset)
{
	offset.p.y += height * 0.5f;
	return createCylinder(height, radius, offset);
}

CollsionShapeDesc CollsionShapeDesc::createCone(const float height, const float radius, const transf3d& offset)
{
	CollsionShapeDesc r;
	r.type = type_cone;
	r.coneHeight = height * offset.s.y;
	r.coneRadius = radius * offset.s.x0z().hsum() / 2.f;
	r.offset = transf3d(offset.p, offset.r, vec3f(1.f));

	return r;
}

CollsionShapeDesc CollsionShapeDesc::createConvexPoly(std::vector<vec3f> verts, std::vector<int> indices)
{
	CollsionShapeDesc r;
	r.type = type_convexPoly;

	r.verticesConvexOrTriMesh = std::move(verts);
	r.indicesConvexOrTriMesh = std::move(indices);

	return r;
}

CollsionShapeDesc CollsionShapeDesc::createTriMesh(std::vector<vec3f> verts, std::vector<int> indices)
{
	CollsionShapeDesc r;
	r.type = type_triangleMesh;

	r.verticesConvexOrTriMesh = std::move(verts);
	r.indicesConvexOrTriMesh = std::move(indices);

	return r;
}

CollsionShapeDesc CollsionShapeDesc::createInfinitePlane(vec3f planeNormal, float planeConstant)
{
	CollsionShapeDesc r;
	r.type = type_infinitePlane;

	r.infinitePlaneNormal = planeNormal;
	r.infinitePlaneConst = planeConstant;

	return r;
}

//-------------------------------------------------------------------------
// CollisionShape
//-------------------------------------------------------------------------
void CollisionShape::create(const CollsionShapeDesc* shapeDescriptors, const int numShapeDescriptors)
{
	destroy();
	m_desc.clear();
	m_desc.reserve(numShapeDescriptors);

	for (int t = 0; t < numShapeDescriptors; ++t) {
		m_desc.push_back(shapeDescriptors[t]);
	}

	struct CreatedShape {
		btCollisionShape* shape = nullptr;
		transf3d offset;
	};

	std::vector<CreatedShape> createdShapes;

	for (CollsionShapeDesc& desc : m_desc) {
		CreatedShape createdShape;

		createdShape.offset = desc.offset;

		switch (desc.type) {
			case CollsionShapeDesc::type_box: {
				createdShape.shape = new btBoxShape(toBullet(desc.boxHalfDiagonal));
			} break;
			case CollsionShapeDesc::type_sphere: {
				createdShape.shape = new btSphereShape(desc.sphereRadius);
			} break;
			case CollsionShapeDesc::type_capsule: {
				createdShape.shape = new btCapsuleShape(desc.capsuleRadius, desc.capsuleHeight);
			} break;
			case CollsionShapeDesc::type_cylinder: {
				createdShape.shape = new btCylinderShape(toBullet(desc.cylinderHalfDiagonal));
			} break;
			case CollsionShapeDesc::type_cone: {
				createdShape.shape = new btConeShape(desc.coneRadius, desc.coneHeight);
			} break;
			case CollsionShapeDesc::type_convexPoly: {
				createdShape.shape = new btConvexHullShape((float*)desc.verticesConvexOrTriMesh.data(),
				                                           int(desc.verticesConvexOrTriMesh.size()), sizeof(vec3f));

				// Caution: [CONVEX_HULLS_TRIANGLE_USER_DATA]
				// The user point here specifies the triangle mesh used
				// to create the convexhull. Bullet doesn't provide a way to store the triangles inside btConvexHullShape
				// however these triangles are needed for the navmesh building.
				createdShape.shape->setUserPointer(&desc);
			} break;
			case CollsionShapeDesc::type_triangleMesh: {
				btTriangleMesh* triMesh = new btTriangleMesh(true, false);
				m_triangleMeshes.push_back(std::make_unique<btTriangleMesh>(triMesh));

				const int numTriangles = int(desc.indicesConvexOrTriMesh.size()) / 3;

				for (int t = 0; t < numTriangles; ++t) {
					const int i0 = desc.indicesConvexOrTriMesh[t * 3 + 0];
					const int i1 = desc.indicesConvexOrTriMesh[t * 3 + 1];
					const int i2 = desc.indicesConvexOrTriMesh[t * 3 + 2];

					const btVector3 v0 = toBullet(desc.verticesConvexOrTriMesh[i0]);
					const btVector3 v1 = toBullet(desc.verticesConvexOrTriMesh[i1]);
					const btVector3 v2 = toBullet(desc.verticesConvexOrTriMesh[i2]);

					triMesh->addTriangle(v0, v1, v2, true);
				}

				createdShape.shape = new btBvhTriangleMeshShape(triMesh, true);

			} break;
			case CollsionShapeDesc::type_infinitePlane: {
				createdShape.shape = new btStaticPlaneShape(toBullet(desc.infinitePlaneNormal), desc.infinitePlaneConst);
			}

			default: {
				sgeAssert("Collision Shape Type not implemented");
			};
		}

		if (createdShape.shape != nullptr) {
			createdShapes.push_back(createdShape);
		}
	}

	// Shortcut for simple collision shape (which is the common case).
	if (createdShapes.size() == 1) {
		if (createdShapes[0].offset == transf3d()) {
			m_btShape.reset(createdShapes[0].shape);
		}
		else {
			btCompoundShape* const compound = new btCompoundShape();
			compound->addChildShape(toBullet(createdShapes[0].offset), createdShapes[0].shape);
			m_btShape.reset(compound);
		}
	}
	else {
		btCompoundShape* const compound = new btCompoundShape();
		for (const CreatedShape& shape : createdShapes) {
			btTransform localTransform = toBullet(shape.offset);
			compound->addChildShape(localTransform, shape.shape);
		}

		m_btShape.reset(compound);
	}
}

} // namespace sge
