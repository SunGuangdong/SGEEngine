
#include "ACandy.h"
#include "AWitch.h"
#include "sge_audio/AudioDevice.h"
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
	    "assets/candy/bon1.png", "assets/candy/bon2.png", "assets/candy/bon3.png", "assets/candy/bon4.png",
	    "assets/candy/bon5.png", "assets/candy/bon6.png", "assets/candy/bon7.png",
	};

	const int texIdx = g_rnd.nextIntBefore(SGE_ARRSZ(candiesTex));
	auto assetTex = getCore()->getAssetLib()->getAsset(candiesTex[texIdx], true);
	ttSprite.images.resize(1);
	ttSprite.images[0].m_assetProperty.setAsset(assetTex);
	ttSprite.images[0].imageSettings.defaultFacingAxisZ = false;
	ttSprite.images[0].imageSettings.m_anchor = anchor_mid;
	ttSprite.images[0].imageSettings.flipHorizontally = g_rnd.nextBool();
	ttSprite.images[0].m_additionalTransform = mat4f::getTranslation(0.f, 4.f, 0.f) * mat4f::getScaling(0.75f);

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
				w->numCandiesCollected += 1;
				getWorld()->setParentOf(getId(), ObjectId());

				if (!isMobileEmscripten) {
					getCore()->getAudioDevice()->play(&sndPickUp, true);
				}
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

				ttSprite.images[0].m_additionalTransform =
				    mat4f::getTranslation(0.f, 4.f, 0.f) *
				    mat4f::getScaling(0.75f) * mat4f::getRotationX(sqr(timeSpendPickedUp / kChaseDuration) * two_pi() * 3.f);

				setTransform(newTransf);
			}

			if (timeSpendPickedUp > kChaseDuration) {
				getWorld()->objectDelete(getId());
			}

			timeSpendPickedUp += u.dt;
		} else {
			ttSprite.images[0].m_additionalTransform =
			    mat4f::getTranslation(vec3f(0.f, 1.f + sinf(getWorld()->timeSpendPlaying * 3.14f * 2.f) * 0.2f, 0.f));
		}
	}
}

} // namespace sge
