#pragma once

#include "sge_engine/sge_engine_api.h"
#include "sge_utils/sge_utils.h"

SGE_NO_WARN_BEGIN
#include <BulletCollision/CollisionDispatch/btGhostObject.h>
#include <btBulletDynamicsCommon.h>
SGE_NO_WARN_END

namespace sge {

/// Computes the needed vector for the @ghostObj to stop penetrating any other objects.
/// @param [out] recoveryVector an optional output if there was any penetratation.
/// @param [in] collisionWorld is the bullet collision world owning the objects.
/// @param [in] collisionObject is the object that would be moved.
/// @param [in] maxPenetrationDepth is the maximum allowed penetration, the physics engine needs some slack when solving collision.
/// @return true if there was any penetration.
///
/// The code is based on the code in Bullet Physics, see btKinematicCharacterController::recoverFromPenetration.
bool SGE_ENGINE_API computePenerationRecorverVectorFromManifold(btVector3* const recoveryVector,
                                                                const btPersistentManifold* persistManifold,
                                                                const btCollisionObject* collisionObject,
                                                                float maxPenetrationDepth = 0.02f);

/// Computes the needed vector for the @ghostObj to stop penetrating any other objects.
/// @param [out] recoveryVector an optional output if there was any penetratation.
/// @param [in] collisionWorld is the bullet collision world owning the objects.
/// @param [in] ghostObj is the object that would be moved.
/// @param [in/out] tempManifoldArray is just an array, storing the manifolds during the computation.
/// 	                              in order to avoid needless alocation (as it is like std::vector)
///                                   it is required by the caller to create one and pass it here.
/// 	                              No previous state is required for it, the function manages it iteself.
/// @param [in] maxPenetrationDepth is the maximum allowed penetration, the physics engine needs some slack when solving collision.
/// @return true if there was any penetration.
///
/// The code is based on the code in Bullet Physics, see btKinematicCharacterController::recoverFromPenetration.
bool SGE_ENGINE_API recoverFromPenetrationVector(btVector3* const recoveryVector,
                                                 btCollisionWorld* collisionWorld,
                                                 btPairCachingGhostObject* ghostObj,
                                                 btManifoldArray& tempManifoldArray,
                                                 float maxPenetrationDepth = 0.02f);


bool SGE_ENGINE_API recoverFromPenetrationVector(btVector3* const recoveryVector,
                                                 btCollisionWorld* collisionWorld,
                                                 btCollisionObject* colObj,
                                                 float maxPenetrationDepth = 0.02f);


} // namespace sge
