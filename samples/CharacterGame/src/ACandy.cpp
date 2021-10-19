
#include "ACandy.h"
#include "AWitch.h"
#include "sge_core/ICore.h"
#include "sge_engine/GameWorld.h"
#include "sge_utils/math/Random.h"

#include "GlobalRandom.h"

namespace sge {

RelfAddTypeId(ACandy, 21'10'16'0001);
ReflBlock() {
	ReflAddActor(ACandy);
}

void ACandy::create() {
	registerTrait(ttSprite);
	registerTrait(ttRigidBody);

	const char* candiesTex[] = {
	    "assets/candy/c0.png", "assets/candy/c1.png", "assets/candy/c2.png", "assets/candy/c3.png", "assets/candy/c4.png",
	};

	const int texIdx = g_rnd.nextIntBefore(SGE_ARRSZ(candiesTex));
	auto assetTex = getCore()->getAssetLib()->getAsset(candiesTex[texIdx], true);
	ttSprite.images.resize(1);
	ttSprite.images[0].m_assetProperty.setAsset(assetTex);
	ttSprite.images[0].imageSettings.defaultFacingAxisZ = false;
	ttSprite.images[0].imageSettings.m_anchor = anchor_bottomMid;
	ttSprite.images[0].m_additionalTransform = mat4f::getTranslation(0.f, 3.f, 0.f);

	ttRigidBody.getRigidBody()->create(this, CollsionShapeDesc::createCylinderBottomAligned(6.f, 1.f), 1.f, true);
	ttRigidBody.getRigidBody()->setCanRotate(false, false, false);
	ttRigidBody.getRigidBody()->setFriction(0.f);
}

void ACandy::update(const GameUpdateSets& u) {
	if (u.isGamePaused()) {
		return;
	}

	if (getPosition().x < 0.f) {
		getWorld()->objectDelete(getId());
	}

	if (isPickedUp == false) {
		if (AWitch* w = getWorld()->getFistObject<AWitch>()) {
			if (w->getPosition().distance(getPosition()) < 3.f) {
				isPickedUp = true;

				getWorld()->setParentOf(getId(), ObjectId());
			}
		}
	}

	if (!isPickedUp) {
		if (AWitch* witch = getWorld()->getFistObject<AWitch>()) {
			ttRigidBody.getRigidBody()->setLinearVelocity(vec3f(-witch->currentSpeedX, 0.f, 0.f));
		}
	} else {
		ttRigidBody.getRigidBody()->setLinearVelocity(vec3f(0, 0.f, 0.f));

		const float kChaseDuration = 1.5f;

		if (AWitch* w = getWorld()->getFistObject<AWitch>()) {
			if (ICamera* cam = getWorld()->getRenderCamera()) {
				vec3f targetPosWs = w->getTransform().p + vec3f(0.2f, 3.f, 0.f);
				vec3f currentPosWs = getTransform().p;
				vec3f newPosWs = speedLerp(currentPosWs, targetPosWs, sqr(timeSpendPickedUp * 2.f) * u.dt * 10.f);

				transf3d newTransf = getTransform();
				newTransf.p = newPosWs;
				newTransf.s = vec3f(1.f - timeSpendPickedUp / kChaseDuration);
				setTransform(newTransf);
			}

			if (timeSpendPickedUp > kChaseDuration) {
				getWorld()->objectDelete(getId());
			}

			timeSpendPickedUp += u.dt;
		} else {
			ttSprite.images[0].m_additionalTransform =
			    mat4f::getTranslation(vec3f(0.f, 1.f + sinf(getWorld()->timeSpendPlaying * 3.14f * 2.f) * 0.5f, 0.f));
		}
	}
}

} // namespace sge
