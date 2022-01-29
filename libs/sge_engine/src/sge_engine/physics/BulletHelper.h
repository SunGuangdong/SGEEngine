#pragma once

#include "sge_utils/math/transform.h"
#include "sge_utils/sge_utils.h"

SGE_NO_WARN_BEGIN
#include <BulletCollision/CollisionShapes/btCollisionShape.h>
#include <LinearMath/btQuaternion.h>
#include <LinearMath/btTransform.h>
#include <LinearMath/btVector3.h>
SGE_NO_WARN_END

namespace sge {

inline btVector3 toBullet(const vec3f& v)
{
	return btVector3(v.x, v.y, v.z);
}

inline vec3f fromBullet(const btVector3& bt)
{
	return vec3f(bt.x(), bt.y(), bt.z());
}

inline btQuaternion toBullet(const quatf v)
{
	return btQuaternion(v.x, v.y, v.z, v.w);
}

inline quatf fromBullet(const btQuaternion& bt)
{
	return quatf(bt.x(), bt.y(), bt.z(), bt.w());
}

inline btTransform toBullet(const transf3d& tr)
{
	return btTransform(toBullet(tr.r), toBullet(tr.p));
}

/// Caution:
/// Scaling in bullet is applied on the rigid body itself, it's not part of the transform!
inline transf3d fromBullet(const btTransform& btTr)
{
	btVector3 const btP = btTr.getOrigin();
	btQuaternion btR;
	btTr.getBasis().getRotation(btR);

	const transf3d res = transf3d(fromBullet(btP), fromBullet(btR), vec3f(1.f));
	return res;
}

/// Caution:
/// As by default Bullet Physics is built with no RTTI support we cannot use dynamic cast.
/// In order to determine the shape of the object we need to use the enum specifying the type of the shape.
template <typename T>
T* btCollisionShapeCast(btCollisionShape* const shape, const BroadphaseNativeTypes type)
{
	if (shape && shape->getShapeType() == type) {
		return static_cast<T*>(shape);
	}

	return nullptr;
}

/// Caution:
/// As by default Bullet Physics is built with no RTTI support we cannot use dynamic cast.
/// In order to determin the shape of the object we need to use the enum specifying the type of the shape.
template <typename T>
const T* btCollisionShapeCast(const btCollisionShape* const shape, const BroadphaseNativeTypes type)
{
	if (shape && shape->getShapeType() == type) {
		return static_cast<const T*>(shape);
	}

	return nullptr;
}

} // namespace sge
