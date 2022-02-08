#pragma once

#include "sge_core/AssetLibrary/AssetLibrary.h"
#include "sge_core/model/ModelAnimator.h"
#include "sge_engine/Actor.h"
#include "sge_engine/actors/ACamera.h"
#include "sge_engine/behaviours/KinematicCharacterCtrl.h"
#include "sge_engine/physics/PhysicsAction.h"
#include "sge_engine/traits/TraitModel.h"
#include "sge_engine/traits/TraitRigidBody.h"
#include "sge_engine/traits/TraitSprite.h"

namespace sge {

struct AGhostObject;
struct GhostAction : PhysicsAction {
	virtual void updateAction(btCollisionWorld* collisionWorld, btScalar deltaTimeStep) override;

	AGhostObject* owner;
	btManifoldArray tempManifoldArr;
};


struct CameraFollowMe {
	ObjectId cameraId;
	// How far the camera can be.
	float maxDistanceHorizontal = 20.f;
	// How closes the camera can be.
	float minDistanceHorizontal = 10.f;

	// How higher should the camera strive to be above the object that we follow.
	float cameraElevationY = 5.f;

	float minDistanceY = 5.f;
	float maxDistanceY = 10.f;

	void update(GameWorld& world, float dt, const vec3f& targetPosition, const vec2f& rotationAroundAndUp);

	vec3f inputToWorldSpace(GameWorld* world, const vec2f inputXZ) const;
};

//--------------------------------------------------------------------
// AStaticObstacle
//--------------------------------------------------------------------
struct SGE_ENGINE_API AGhostObject : public Actor {
	void create() final;
	void postUpdate(const GameUpdateSets& updateSets) final;
	Box3f getBBoxOS() const final;

	void onPlayStateChanged(bool const isStartingToPlay) override;

  public:
	TraitRigidBody m_traitRB;
	TraitModel m_traitModel;
	GhostAction action;
	CharacterCtrlKinematic charCtrl;
	ModelAnimator animator;
	CameraFollowMe cameraFollowMe;

	ObjectId cameraId;

	vec3f inputWorldSpace = vec3f(0.f);
	bool jumpPressed = false;
	bool jumpReleased = false;
	bool isCharCtrlInputHandled = false;
};


} // namespace sge
