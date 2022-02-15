#include "AGhostObject.h"
#include "sge_core/GeomGen.h"
#include "sge_core/ICore.h"
#include "sge_engine/GameWorld.h"
#include "sge_engine/RigidBodyFromModel.h"
#include "sge_engine/physics/PenetrationRecovery.h"
#include "sge_utils/math/EulerAngles.h"


namespace sge {

struct ACoin : public Actor {
	TraitModel ttModel;
	TraitRigidBody ttRb;

	Box3f getBBoxOS() const
	{
		Box3f bbox = ttModel.getBBoxOS();
		return bbox;
	}

	void create()
	{
		registerTrait(ttModel);
		registerTrait(ttRb);
		ttModel.addModel("assets/coin.mdl");
		ttRb.createBasedOnModel("assets/coin.mdl", 0.f, true, false);
	}

	void update(const GameUpdateSets& u)
	{
		if (u.isSimPaused) {
			return;
		}

		const auto& itr = getWorld()->m_physicsManifoldList.find(ttRb.getRigidBody());

		if (itr != getWorld()->m_physicsManifoldList.end()) {
			getWorld()->objectDelete(getId());
		}
	}
};

ReflBlock()
{
	ReflAddActor(ACoin);
}

void GhostAction::updateAction(btCollisionWorld* UNUSED(collisionWorld), btScalar deltaTimeStep)
{
	btPairCachingGhostObject* ghostObj = owner->m_traitRB.getRigidBody()->getBulletGhostObject();
	if (ghostObj == nullptr) {
		return;
	}

	CharacterCtrlInput input = {
	    -owner->getDirZ(),
	    owner->inputWorldSpace,
	    owner->jumpPressed && !owner->isCharCtrlInputHandled,
	    owner->jumpReleased && !owner->isCharCtrlInputHandled,
	};

	owner->charCtrl.update(input, *owner->m_traitRB.getRigidBody(), deltaTimeStep);

	owner->isCharCtrlInputHandled = true;
}

//--------------------------------------------------------------------
// AStaticObstacle
//--------------------------------------------------------------------
// clang-format off
ReflAddTypeId(AGhostObject, 22'01'29'0001);
ReflAddTypeId(CameraFollowMe, 22'02'06'0001);
ReflBlock()
{
	ReflAddActor(AGhostObject) 
		ReflMember(AGhostObject, m_traitModel)
		ReflMember(AGhostObject, cameraFollowMe)
	;

	ReflAddType(CameraFollowMe)
		ReflMember(CameraFollowMe, cameraId)
		ReflMember(CameraFollowMe, maxDistanceHorizontal)
		ReflMember(CameraFollowMe, maxDistanceHorizontal)
		ReflMember(CameraFollowMe, cameraElevationY)
		ReflMember(CameraFollowMe, minDistanceY)
		ReflMember(CameraFollowMe, maxDistanceY)
	;
}
// clang-format on

Box3f AGhostObject::getBBoxOS() const
{
	Box3f bbox = m_traitModel.getBBoxOS();
	return bbox;
}

void AGhostObject::create()
{
	registerTrait(m_traitRB);
	registerTrait(m_traitModel);

	CollsionShapeDesc shapeDesc = CollsionShapeDesc::createCapsule(4.f, 1.f, transf3d(vec3f(0.f, 3.f, 0.f)));

	m_traitRB.getRigidBody()->createGhost((Actor*)this, &shapeDesc, 1, true);

	m_traitModel.uiDontOfferResizingModelCount = false;
	m_traitModel.m_models.resize(1);
	m_traitModel.m_models[0].setModel("assets/knight/knight.mdl", true);

	animator.create(*m_traitModel.m_models[0].customEvalModel->m_model);
	animator.trackAddAmim(0, nullptr, "idle");
	animator.trackSetFadeTime(0, 0.2f);
	animator.trackAddAmim(1, nullptr, "walk");
	animator.trackSetFadeTime(1, 0.2f);
	animator.trackAddAmim(2, nullptr, "walk");
	animator.trackSetFadeTime(2, 0.2f);

	animator.playTrack(0);

	charCtrl.m_cfg.defaultFacingDir = vec3f(0.f, 0.f, -1.f);
	charCtrl.m_cfg.feetLevel = 1.f;

	action.owner = this;
}

void AGhostObject::onPlayStateChanged(bool const isStartingToPlay)
{
	if (isStartingToPlay) {
		getWorld()->physicsWorld.dynamicsWorld->addAction(&action);
		getWorld()->physicsWorld.addPhysicsObject(m_traitRB.m_rigidBody);
	}
	else {
		getWorld()->physicsWorld.dynamicsWorld->removeAction(&action);
		getWorld()->physicsWorld.removePhysicsObject(m_traitRB.m_rigidBody);
	}
}

void AGhostObject::update(const GameUpdateSets& updateSets)
{
	animator.advanceAnimation(updateSets.dt);
	std::vector<mat4f> transfBones;

	animator.computeModleNodesTrasnforms(transfBones);
	m_traitModel.m_models[0].customEvalModel->evaluate(transfBones.data(), int(transfBones.size()));

	isCharCtrlInputHandled = false;
	inputWorldSpace = cameraFollowMe.inputToWorldSpace(getWorld(), updateSets.is.GetArrowKeysDir(true, false, 0));
	jumpPressed = updateSets.is.IsKeyPressed(Key_X);
	jumpReleased = updateSets.is.IsKeyReleased(Key_X);


	if (charCtrl.isJumping) {
		animator.playTrack(2);
	}
	else if (inputWorldSpace.lengthSqr()) {
		animator.playTrack(1);
	}
	else {
		animator.playTrack(0);
	}

	this->setOrientation(charCtrl.facingRotation, false);

	vec2f rotation = vec2f(0.f);
	if (const GamepadState* gs = updateSets.is.getHookedGemepad(0)) {
		rotation.x = -gs->axisR.x * deg2rad(180.f) * updateSets.dt;
		rotation.y = -gs->axisR.y * deg2rad(180.f) * updateSets.dt;

		jumpPressed = gs->isBtnPressed(GamepadState::btn_a);
		jumpReleased = gs->isBtnReleased(GamepadState::btn_a);
	}

	cameraFollowMe.update(*getWorld(), updateSets.dt, getPosition(), rotation);
}

void CameraFollowMe::update(
    GameWorld& world, float UNUSED(dt), const vec3f& targetPosition, const vec2f& rotationAroundAndUp)
{
	ACamera* camera = world.getActor<ACamera>(cameraId);
	if (!camera) {
		return;
	}

	vec3f newCameraPosition = camera->getPosition();
	quatf newCameraOrientation = camera->getOrientation();


	vec3f targetToCamera = camera->getPosition() - targetPosition;

	// Rotation form input.
	{
		if (rotationAroundAndUp.x != 0.f) {
			targetToCamera = rotateAround(vec3f(0.f, 1.f, 0.f), rotationAroundAndUp.x, targetToCamera);
			newCameraPosition = targetPosition + targetToCamera;
		}

		if (rotationAroundAndUp.y != 0.f) {
			targetToCamera = rotateAround(camera->getDirX(), rotationAroundAndUp.y, targetToCamera);
			newCameraPosition = targetPosition + targetToCamera;
		}
	}


	const vec3f targetToCameraHorizontal = targetToCamera.x0z();

	// Horizontal Position.
	{
		const float horizontalDistance = targetToCameraHorizontal.length();
		if (horizontalDistance > maxDistanceHorizontal) {
			// If the camera is too far horizontally, bring her closer.
			newCameraPosition -= (horizontalDistance - maxDistanceHorizontal) * targetToCameraHorizontal.normalized0();
		}
		else if (horizontalDistance < minDistanceHorizontal) {
			// If the camera is too close horizontally, push her away.
			newCameraPosition += (minDistanceHorizontal - horizontalDistance) * targetToCameraHorizontal.normalized0();
		}
	}

	// Vertical Position.
	{
		if (newCameraPosition.y < targetPosition.y + cameraElevationY - minDistanceY) {
			newCameraPosition.y = targetPosition.y + cameraElevationY - minDistanceY;
		}

		if (newCameraPosition.y > targetPosition.y + cameraElevationY + maxDistanceY) {
			newCameraPosition.y = targetPosition.y + cameraElevationY + maxDistanceY;
		}
	}

	// Camera orientation.
	{
		vec3f defaultCameraLookDir = vec3f(0.f, 0.f, -1.f);
		vec3f targetLookDir = normalized0(targetPosition - newCameraPosition);

		vec3f targetLookDirHorizontal = normalized0(targetLookDir.x0z());

		quatf orientation =
		    quatf::fromNormalizedVectors(defaultCameraLookDir, targetLookDirHorizontal, vec3f(0.f, 1.f, 0.f));
		orientation =
		    quatf::fromNormalizedVectors(targetLookDirHorizontal, targetLookDir, vec3f(1.f, 0.f, 0.f)) * orientation;

		camera->setPosition(newCameraPosition);
		camera->setOrientation(orientation);
	}
}


vec3f CameraFollowMe::inputToWorldSpace(GameWorld* world, const vec2f inputXZ) const
{
	ACamera* camera = world->getActor<ACamera>(cameraId);
	if (!camera) {
		return vec3f(inputXZ.x, 0.f, inputXZ.y);
	}

	vec3f cameraForward = -camera->getDirZ();
	vec3f cameraRight = camera->getDirX();

	cameraForward.y = 0.f;
	cameraForward = cameraForward.normalized0();

	// if the camera is looking trait down, than use the camera up (Y-axis) as a fallback.
	if (cameraForward == vec3f(0.f, 0.f, 0.f)) {
		cameraForward = camera->getDirY();
		cameraForward.y = 0.f;
		cameraForward = cameraForward.normalized0();
	}

	cameraRight.y = 0.f;
	cameraRight = cameraRight.normalized0();

	// if the camera right isz zero, then compute it using the forward vector.
	if (cameraRight == vec3f(0.f, 0.f, 0.f)) {
		cameraRight = cross(cameraForward, camera->getDirY());
		cameraRight.y = 0;
		cameraRight = cameraRight.normalized0();
	}

	vec3f result = cameraForward * inputXZ.y + cameraRight * inputXZ.x;
	result = result.normalized0() * inputXZ.length();

	return result;
}

} // namespace sge
