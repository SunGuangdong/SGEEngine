
#include "AWitch.h"
#include "ABat.h"
#include "sge_core/ICore.h"
#include "sge_engine/GameWorld.h"
#include "sge_utils/math/Random.h"


namespace sge {

RelfAddTypeId(ABat, 21'10'03'0002);
ReflBlock() {
	ReflAddActor(ABat);
}

Random g_batRandom;

void ABat::create() {
	registerTrait(ttSprite);
	registerTrait(ttRigidBody);

	const char* batFrames[] = {
	    "assets/bats/B1.png", "assets/bats/B1-2.png", "assets/bats/B2.png", "assets/bats/B2-2.png",
	    "assets/bats/B3.png", "assets/bats/B3-2.png", "assets/bats/B4.png", "assets/bats/B4-2.png",
	};

	int frame0 = (g_batRandom.nextInt() % (SGE_ARRSZ(batFrames) / 2)) * 2;

	frames[0] = getCore()->getAssetLib()->getAsset(batFrames[frame0], true);
	frames[1] = getCore()->getAssetLib()->getAsset(batFrames[frame0 + 1], true);

	ttSprite.images.resize(1);
	ttSprite.images[0].m_assetProperty.setAsset(frames[0]);
	ttSprite.images[0].imageSettings.defaultFacingAxisZ = false;
	ttSprite.images[0].imageSettings.m_anchor = anchor_bottomMid;
	ttSprite.images[0].m_additionalTransform = mat4f::getTranslation(0.f, 3.f, 0.f);

	ttRigidBody.getRigidBody()->create(this, CollsionShapeDesc::createCylinderBottomAligned(6.f, 1.f), 0.f, true);
	ttRigidBody.getRigidBody()->setCanRotate(false, false, false);
	ttRigidBody.getRigidBody()->setFriction(0.f);
}

void ABat::update(const GameUpdateSets& u) {
	if (u.isGamePaused()) {
		return;
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

	if (getPosition().x < 100.f) {
		vec3f newPos = getPosition();
		newPos.x -= 10.f * u.dt;
		setPosition(newPos);
	}

	ttSprite.images[0].m_additionalTransform =
	    mat4f::getTranslation(vec3f(0.f, 3.f + sinf(getWorld()->timeSpendPlaying * 3.14f * 2.f) * 0.5f, 0.f));

	int iFrame = int(getWorld()->timeSpendPlaying * 1000.f) % 500 > 250 ? 0 : 1;
	iFrame += (getId().id % 2);
	iFrame = iFrame % 2;
	ttSprite.images[0].m_assetProperty.setAsset(frames[iFrame]);
}

} // namespace sge
