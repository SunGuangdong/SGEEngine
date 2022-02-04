#pragma once

#include "sge_utils/sge_utils.h"

SGE_NO_WARN_BEGIN
#include <BulletCollision/CollisionDispatch/btCollisionDispatcher.h>
SGE_NO_WARN_END
class btCollisionConfiguration;

namespace sge {

/// SgeCollisionDispatcher is an extention to the defautly used btCollisionDispatcher.
/// We particularly take interset in this class as it allow us to filter collisions.
struct SgeCollisionDispatcher : public btCollisionDispatcher {
	SgeCollisionDispatcher(btCollisionConfiguration* collisionConfiguration)
	    : btCollisionDispatcher(collisionConfiguration)
	{
	}

	bool needsCollision(const btCollisionObject* body0, const btCollisionObject* body1) override;

	bool needsResponse(const btCollisionObject* body0, const btCollisionObject* body1) override;
};
} // namespace sge
