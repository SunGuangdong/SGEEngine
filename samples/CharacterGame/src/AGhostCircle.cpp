#include "AGhostCircle.h"
#include "AWitch.h"
#include "sge_core/ICore.h"
#include "sge_engine/GameWorld.h"
#include "sge_utils/math/Random.h"

namespace sge {

RelfAddTypeId(AGhostCircle, 21'10'14'0001);
ReflBlock() {
	ReflAddActor(AGhostCircle);
}

Random g_ghostCircleRandom;

static inline const int kNumGhosts = 4;
static inline const float circleRadius = 2.5f;

void AGhostCircle::create() {
	registerTrait(ttSprite);
	registerTrait(ttRigidBody);

	const char* ghostTexs[] = {
	    "assets/ghosts/G1.png", "assets/ghosts/G2.png",  "assets/ghosts/G3.png",  "assets/ghosts/G4.png",
	    "assets/ghosts/G5.png", "assets/ghosts/G6.png",  "assets/ghosts/G7.png",  "assets/ghosts/G8.png",
	    "assets/ghosts/G9.png", "assets/ghosts/G10.png", "assets/ghosts/G11.png",
	};

	ttSprite.hasShadows = false;
	ttSprite.images.resize(kNumGhosts);

	for (int t = 0; t < kNumGhosts; ++t) {
		const float angleDiff = two_pi() / float(kNumGhosts);

		
		TraitSprite::Element& image = ttSprite.images[t];

		vec3f ghostPosOs = vec3f(sin(angleDiff * t), 0.f, cos(angleDiff * t)) * circleRadius;
		image.m_additionalTransform = mat4f::getTranslation(ghostPosOs) * mat4f::getScaling(1.5f);

		image.imageSettings.defaultFacingAxisZ = false;
		image.imageSettings.m_anchor = anchor_bottomMid;
		image.imageSettings.m_billboarding = billboarding_yOnly;
		image.imageSettings.forceAlphaBlending = true;
		image.imageSettings.colorTint = vec4f(94.f / 255.f, 239.f / 255.f, 251.f / 255.f, 1.f);

		int frame0 = (g_ghostCircleRandom.nextInt() % (SGE_ARRSZ(ghostTexs)));
		auto ghostImg = getCore()->getAssetLib()->getAsset(ghostTexs[frame0], true);

		image.m_assetProperty.setAsset(ghostImg);
	}

	ttRigidBody.getRigidBody()->create(this, CollsionShapeDesc::createCylinderBottomAligned(6.f, 3.f), 1.f, true);
	ttRigidBody.getRigidBody()->setCanRotate(false, false, false);
	ttRigidBody.getRigidBody()->setFriction(0.f);
}

void AGhostCircle::update(const GameUpdateSets& u) {
	if (u.isGamePaused()) {
		return;
	}

	if (getPosition().x < 0.f) {
		getWorld()->objectDelete(getId());
	}

	for (int t = 0; t < kNumGhosts; ++t) {
		TraitSprite::Element& image = ttSprite.images[t];

		const float ghostAngle = t * two_pi() / float(kNumGhosts) + getWorld()->timeSpendPlaying * two_pi() * 0.5f;

		vec3f ghostPosOs = vec3f(sin(ghostAngle), 0.f, cos(ghostAngle)) * circleRadius;
		image.m_additionalTransform = mat4f::getTranslation(ghostPosOs) * mat4f::getScaling(1.5f);
	}

	if (getPosition().x < 100.f) {
		if (AWitch* witch = getWorld()->getFistObject<AWitch>()) {
			vec3f speed = vec3f(-witch->currentSpeedX, 0.f, 9.f * (isGoingPosZ ? 1.f : -1.f));
			ttRigidBody.getRigidBody()->setLinearVelocity(speed);
		}

		for (const btPersistentManifold* manifold : getWorld()->getRigidBodyManifolds(ttRigidBody.getRigidBody())) {
			if (manifold) {
				[[maybe_unused]] Actor* other = getOtherActorFromManifoldMutable(manifold, this);
				if (other && other->getType() == sgeTypeId(AWitch)) {
					((AWitch*)other)->applyDamage();
					break;
				}
			}
		}
	} else {
		isGoingPosZ = getPosition().z < 0.f;

		AWitch* witch = getWorld()->getFistObject<AWitch>();
		if (witch) {
			ttRigidBody.getRigidBody()->setLinearVelocity(vec3f(-witch->currentSpeedX, 0.f, 0.f));
		}
	}
}


RelfAddTypeId(ALeftRightGhost, 21'10'23'0001);
ReflBlock() {
	ReflAddActor(ALeftRightGhost);
}

void ALeftRightGhost::create() {
	registerTrait(ttSprite);
	registerTrait(ttRigidBody);

	const char* ghostTexs[] = {
	    "assets/ghosts/G1.png",
	    "assets/ghosts/G4.png",
	    "assets/ghosts/G10.png",
	};

	ttSprite.hasShadows = false;
	ttSprite.images.resize(1);
	ttSprite.images[0].imageSettings.defaultFacingAxisZ = false;
	ttSprite.images[0].imageSettings.m_anchor = anchor_bottomMid;
	ttSprite.images[0].imageSettings.m_billboarding = billboarding_yOnly;
	ttSprite.images[0].imageSettings.forceAlphaBlending = true;
	ttSprite.images[0].imageSettings.colorTint = vec4f(94.f / 255.f, 239.f / 255.f, 251.f / 255.f, 1.f);
	ttSprite.images[0].m_additionalTransform = mat4f::getScaling(1.5f);
	int frame0 = (g_ghostCircleRandom.nextInt() % (SGE_ARRSZ(ghostTexs)));
	auto ghostImg = getCore()->getAssetLib()->getAsset(ghostTexs[frame0], true);

	ttSprite.images[0].m_assetProperty.setAsset(ghostImg);

	ttRigidBody.getRigidBody()->create(this, CollsionShapeDesc::createCylinderBottomAligned(6.f, 1.f), 1.f, true);
	ttRigidBody.getRigidBody()->setCanRotate(false, false, false);
	ttRigidBody.getRigidBody()->setFriction(0.f);
}

void ALeftRightGhost::update(const GameUpdateSets& u) {
	if (u.isGamePaused()) {
		return;
	}

	if (getPosition().x < 0.f) {
		getWorld()->objectDelete(getId());
	}

	if (!isInitalPickDirDone) {
		isGoingPosZ = getPosition().z < 0.f;
		isInitalPickDirDone = true;
	}

	if (fabsf(getPosition().z) >= 7.5f) {
		isGoingPosZ = getPosition().z < 0.f;
	}


	if (AWitch* witch = getWorld()->getFistObject<AWitch>()) {
		vec3f speed = vec3f(-witch->currentSpeedX, 0.f, 6.f * (isGoingPosZ ? 1.f : -1.f));
		ttRigidBody.getRigidBody()->setLinearVelocity(speed);
	}

	for (const btPersistentManifold* manifold : getWorld()->getRigidBodyManifolds(ttRigidBody.getRigidBody())) {
		if (manifold) {
			[[maybe_unused]] Actor* other = getOtherActorFromManifoldMutable(manifold, this);
			if (other && other->getType() == sgeTypeId(AWitch)) {
				((AWitch*)other)->applyDamage();
				break;
			}
		}
	}
}


} // namespace sge
