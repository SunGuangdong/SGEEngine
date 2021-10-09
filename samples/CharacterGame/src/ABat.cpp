
#include "ABat.h"
#include "sge_core/ICore.h"
#include "sge_engine/GameWorld.h"



namespace sge {

RelfAddTypeId(ABat, 21'10'03'0002);
ReflBlock() {
	ReflAddActor(ABat);
}

void ABat::create() {
	registerTrait(ttSprite);
	registerTrait(ttRigidBody);

	frames[0] = getCore()->getAssetLib()->getAsset("assets/bats/B1.png", true);
	frames[1] = getCore()->getAssetLib()->getAsset("assets/bats/B1-2.png", true);

	ttSprite.m_assetProperty.setAsset(frames[0]);
	ttSprite.imageSettings.defaultFacingAxisZ = false;
	ttSprite.imageSettings.m_anchor = anchor_bottomMid;

	ttRigidBody.getRigidBody()->create(this, CollsionShapeDesc::createSphere(0.75f, transf3d(vec3f(0.f, 1.f, 0.f))), 0.f, true);
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
		}
	}


	int iFrame = int(getWorld()->timeSpendPlaying * 1000.f) % 500 > 250 ? 0 : 1;
	ttSprite.m_assetProperty.setAsset(frames[iFrame]);
}

} // namespace sge
