#include "APumpkinProjectile.h"
#include "sge_core/ICore.h"
#include "sge_engine/GameWorld.h"

namespace sge {

RelfAddTypeId(APumpkinProjectile, 21'09'17'0001);

ReflBlock() {
	ReflAddActor(APumpkinProjectile);
}

void APumpkinProjectile::create() {
	registerTrait(ttRigidbody);
	registerTrait(ttSprite);

	ttRigidbody.getRigidBody()->create(this, CollsionShapeDesc::createSphere(0.5f), 0.f, false);
	ttRigidbody.getRigidBody()->setNoCollisionResponse(true);
	auto imageAsset = getCore()->getAssetLib()->getAsset("assets/pumpkin1.png", true);
	ttSprite.m_assetProperty.setAsset(imageAsset);
	ttSprite.imageSettings.m_anchor = Anchor::anchor_mid;
	ttSprite.imageSettings.defaultFacingAxisZ = false;
}

float easeOutQuartDeriv(float x) {
	return 4.f * powf(1.f - x, 3.f);
}

float easeInCircleDeriv(float x) {
	if (x == 0) {
		return 0.f;
	}
	return x / sqrt(1.f - x);
}

void APumpkinProjectile::postUpdate(const GameUpdateSets& u) {
	const float kFlightSpeedHor = 40.f;

	if (u.isGamePaused()) {
		return;
	}

	 timeSpentAlive += u.dt;

	float speed = easeOutQuartDeriv(timeSpentAlive * 0.5f) * 10.f;
	float vspeed = easeInCircleDeriv(timeSpentAlive) * 5.f;

	this->setPosition(getPosition() + horizontalFlightDir * (u.dt * speed) + vec3f(0.f, -1.f, 0.f) * (vspeed * u.dt));

	passedDistance += speed * u.dt;

	if (passedDistance > 10.f) {
		getWorld()->objectDelete(getId());
	}
}

} // namespace sge
