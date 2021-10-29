
#include "ABat.h"
#include "AWitch.h"
#include "GlobalRandom.h"
#include "sge_core/ICore.h"
#include "sge_engine/GameWorld.h"
#include "sge_utils/math/Random.h"

namespace sge {

RelfAddTypeId(ABat, 21'10'03'0002);
ReflBlock() {
	ReflAddActor(ABat);
}

void ABat::create() {
	registerTrait(ttSprite);
	registerTrait(ttRigidBody);

	const char* batFrames[] = {
	    "assets/bats/B1.png", "assets/bats/B1-2.png", "assets/bats/B2.png", "assets/bats/B2-2.png",
	    "assets/bats/B3.png", "assets/bats/B3-2.png", "assets/bats/B4.png", "assets/bats/B4-2.png",
	};

	int frame0 = (g_rnd.nextInt() % (SGE_ARRSZ(batFrames) / 2)) * 2;

	frames[0] = getCore()->getAssetLib()->getAsset(batFrames[frame0], true);
	frames[1] = getCore()->getAssetLib()->getAsset(batFrames[frame0 + 1], true);

	ttSprite.images.resize(1);
	ttSprite.images[0].m_assetProperty.setAsset(frames[0]);
	ttSprite.images[0].imageSettings.defaultFacingAxisZ = false;
	ttSprite.images[0].imageSettings.m_anchor = anchor_bottomMid;
	ttSprite.images[0].imageSettings.forceAlphaBlending = true;
	ttSprite.images[0].m_additionalTransform = mat4f::getTranslation(0.f, 3.f, 0.f);

	ttRigidBody.getRigidBody()->create(this, CollsionShapeDesc::createCylinderBottomAligned(6.f, 0.75f), 1.f, true);
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

	if (getPosition().x < 0.f) {
		getWorld()->objectDelete(getId());
	}

	if (AWitch* witch = getWorld()->getFistObject<AWitch>()) {
		vec3f speed = vec3f(-witch->currentSpeedX, 0.f, 0.f);
		ttRigidBody.getRigidBody()->setLinearVelocity(speed);
	}

	ttSprite.images[0].m_additionalTransform =
	    mat4f::getTranslation(vec3f(0.f, 3.f + sinf(getWorld()->timeSpendPlaying * 3.14f * 2.f) * 0.5f, 0.f));

	int iFrame = int(getWorld()->timeSpendPlaying * 1000.f) % 500 > 250 ? 0 : 1;
	iFrame += (getId().id % 2);
	iFrame = iFrame % 2;
	ttSprite.images[0].m_assetProperty.setAsset(frames[iFrame]);
}

RelfAddTypeId(APumpkinOnRoad, 21'10'19'0001);
ReflBlock() {
	ReflAddActor(APumpkinOnRoad);
}

void APumpkinOnRoad::create() {
	registerTrait(ttSprite);
	registerTrait(ttRigidBody);


	const char* g_decoresPumpkins[] = {
	    "assets/pumpkins/P3.png",
	    "assets/pumpkins/P5.png",
	    "assets/pumpkins/P10.png",
	};



	ttSprite.images.resize(4);

	float upwardsMovement = 0;
	for (int t = 0; t < 3; ++t) {
		int frame0 = (g_rnd.nextInt() % (SGE_ARRSZ(g_decoresPumpkins)));
		auto asset = getCore()->getAssetLib()->getAsset(g_decoresPumpkins[frame0], true);
		ttSprite.images[t].m_assetProperty.setAsset(asset);
		ttSprite.images[t].imageSettings.defaultFacingAxisZ = false;
		ttSprite.images[t].imageSettings.m_anchor = anchor_bottomMid;
		ttSprite.images[t].imageSettings.forceAlphaBlending = true;
		ttSprite.images[t].imageSettings.flipHorizontally = g_rnd.nextBool();
		ttSprite.images[t].m_additionalTransform = mat4f::getTranslation(-0.02f * float(t), upwardsMovement, 0.f);
		upwardsMovement +=
		    (float(asset->asTextureView()->tex->getDesc().texture2D.height) / ttSprite.images[t].imageSettings.m_pixelsPerUnit) * 0.75f;
	}

	// Shadow
	{
		auto asset = getCore()->getAssetLib()->getAsset("assets/shadow.png", true);
		ttSprite.images[3].m_assetProperty.setAsset(asset);
		ttSprite.images[3].imageSettings.defaultFacingAxisZ = false;
		ttSprite.images[3].imageSettings.m_anchor = anchor_mid;
		ttSprite.images[3].imageSettings.forceAlphaBlending = true;
		ttSprite.images[3].imageSettings.flipHorizontally = g_rnd.nextBool();
		ttSprite.images[3].imageSettings.colorTint.w = 0.5f;
		ttSprite.images[3].m_additionalTransform = mat4f::getScaling(0.5f) * mat4f::getTranslation(0.f, 0.1f, 0.f) * mat4f::getRotationZ(half_pi());
	}

	ttRigidBody.getRigidBody()->create(this, CollsionShapeDesc::createCylinderBottomAligned(6.f, 2.f), 1.f, true);
	ttRigidBody.getRigidBody()->setCanRotate(false, false, false);
	ttRigidBody.getRigidBody()->setFriction(0.f);
}

void APumpkinOnRoad::update(const GameUpdateSets& u) {
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

	if (getPosition().x < 0.f) {
		getWorld()->objectDelete(getId());
	}

	if (AWitch* witch = getWorld()->getFistObject<AWitch>()) {
		vec3f speed = vec3f(-witch->currentSpeedX, 0.f, 0.f);
		ttRigidBody.getRigidBody()->setLinearVelocity(speed);
	}
}

} // namespace sge
