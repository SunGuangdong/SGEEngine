#include "SgeCollisionDispatcher.h"
#include "RigidBody.h"

namespace sge {
bool SgeCollisionDispatcher::needsCollision(const btCollisionObject* body0, const btCollisionObject* body1)
{
	RigidBody* rb0 = (RigidBody*)body0->getUserPointer();
	RigidBody* rb1 = (RigidBody*)body1->getUserPointer();

	bool needsToCollide = true;
	if (rb0 && rb1) {
		bool agree0 = rb0->getMaskIdentifiesAs() & rb1->getMaskCollidesWith();
		bool agree1 = rb1->getMaskIdentifiesAs() & rb0->getMaskCollidesWith();

		needsToCollide = agree0 || agree1;
	}

	if (needsToCollide) {
		return btCollisionDispatcher::needsCollision(body0, body1);
	}
	else {
		return false;
	}
}

bool SgeCollisionDispatcher::needsResponse(const btCollisionObject* body0, const btCollisionObject* body1)
{
	return btCollisionDispatcher::needsResponse(body0, body1);
}
} // namespace sge
