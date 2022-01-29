#pragma once

#include "sge_engine/sge_engine_api.h"
#include "sge_utils/sge_utils.h"

SGE_NO_WARN_BEGIN
#include <BulletDynamics/Dynamics/btActionInterface.h>
SGE_NO_WARN_END

namespace sge {

/// PhysicsAction provides an interfaces that will get invoked
/// on every physics sub step.
/// Particularly useful when writing kinematic objects (like characters)
/// or when you need to do something with the same update rate as the physics (fixedUpdate in other engines).
/// You can still use btActionInterface directly if you want.
struct PhysicsAction : public btActionInterface {
	PhysicsAction() = default;

	// btActionInterface provides the commented method below
	// it is the meat of this class.
	// virtual void updateAction(btCollisionWorld* collisionWorld, btScalar deltaTimeStep) override;

	virtual void debugDraw(btIDebugDraw* UNUSED(debugDrawer)) override
	{
	}
};

} // namespace sge
