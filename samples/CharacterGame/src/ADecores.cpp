#include "ADecores.h"
#include "sge_core/ICore.h"
#include "sge_engine/GameWorld.h"
#include "sge_utils/math/Random.h"

namespace sge {
void ADecoreGhost::create() {
	rnd.setSeed(int(size_t(this)));
	registerTrait(ttSprite);

	auto assetMoon = getCore()->getAssetLib()->getAsset("assets/decores/G9.png", true);
	ttSprite.m_assetProperty.setAsset(assetMoon);
	ttSprite.imageSettings.defaultFacingAxisZ = false;
	ttSprite.imageSettings.m_anchor = anchor_mid;
	ttSprite.imageSettings.forceAlphaBlending = true;
	ttSprite.imageSettings.forceNoWitchGameBending = true;
	ttSprite.imageSettings.colorTint = vec4f(1.f, 1.f, 1.f, 0.98f);

	curvingAngle = rnd.nextInRange(0.2f, deg2rad(4.f)) * (rnd.nextBool() ? 1.f : -1.f);
	speed = rnd.nextInRange(4.f, 13.f);
	lifeSpan = rnd.nextInRange(3.f, 5.f);
}

void ADecoreGhost::update(const GameUpdateSets& u) {
	if (u.isGamePaused()) {
		return;
	}

	if (getPosition().x <= 100.f) {
		vec3f posDiff = this->getDirY() * speed * u.dt;
		setOrientation(quatf::getAxisAngle(vec3f::axis_x(), curvingAngle * u.dt) * getOrientation());
		setPosition(getPosition() + posDiff);

		float fadeStart = lifeSpan - 0.5f;
		if (lifeAccum >= fadeStart) {
			ttSprite.imageSettings.colorTint.w = 1.f - clamp01((lifeAccum - fadeStart) / 0.5f);
		}


		lifeAccum += u.dt;
	}
}

RelfAddTypeId(ADecoreGhost, 21'10'09'0001);
ReflBlock() {
	ReflAddActor(ADecoreGhost);
}

} // namespace sge
