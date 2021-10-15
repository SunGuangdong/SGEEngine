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
		image.imageSettings.colorTint = vec4f(1.f, 1.f, 1.f, 2.f);

		int frame0 = (g_ghostCircleRandom.nextInt() % (SGE_ARRSZ(ghostTexs)));
		auto ghostImg = getCore()->getAssetLib()->getAsset(ghostTexs[frame0], true);

		image.m_assetProperty.setAsset(ghostImg);
	}

	ttRigidBody.getRigidBody()->create(this, CollsionShapeDesc::createCylinderBottomAligned(6.f, 3.f), 0.f, true);
	ttRigidBody.getRigidBody()->setCanRotate(false, false, false);
	ttRigidBody.getRigidBody()->setFriction(0.f);
}

void AGhostCircle::update(const GameUpdateSets& u) {
	if (u.isGamePaused()) {
		return;
	}

	for (int t = 0; t < kNumGhosts; ++t) {
		TraitSprite::Element& image = ttSprite.images[t];

		const float ghostAngle = t * two_pi() / float(kNumGhosts) + getWorld()->timeSpendPlaying * two_pi() * 0.5f;

		vec3f ghostPosOs = vec3f(sin(ghostAngle), 0.f, cos(ghostAngle)) * circleRadius;
		image.m_additionalTransform = mat4f::getTranslation(ghostPosOs) * mat4f::getScaling(1.5f);
	}

	if (getPosition().x < 100.f) {
		vec3f newPos = getPosition();
		newPos.z += 10.f * u.dt * (isGoingPosZ ? 1.f : -1.f);
		setPosition(newPos);

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
	}
}

} // namespace sge
