#include "ADecores.h"
#include "sge_core/ICore.h"
#include "sge_engine/GameWorld.h"
#include "sge_utils/math/Random.h"

namespace sge {
void ADecoreGhost::create() {
	rnd.setSeed(int(size_t(this)));
	registerTrait(ttSprite);

	const char* ghostTexs[] = {
		"assets/ghosts/G1.png",
		"assets/ghosts/G2.png",
		"assets/ghosts/G3.png",
		"assets/ghosts/G4.png",
		"assets/ghosts/G5.png",
		"assets/ghosts/G6.png",
		"assets/ghosts/G7.png",
		"assets/ghosts/G8.png",
		"assets/ghosts/G9.png",
		"assets/ghosts/G10.png",
		"assets/ghosts/G11.png",
	};

	auto assetMoon = getCore()->getAssetLib()->getAsset(ghostTexs[rnd.nextInt() % SGE_ARRSZ(ghostTexs)] , true);
	ttSprite.images.resize(1);
	ttSprite.images[0].m_assetProperty.setAsset(assetMoon);
	ttSprite.images[0].imageSettings.defaultFacingAxisZ = false;
	ttSprite.images[0].imageSettings.m_anchor = anchor_mid;
	ttSprite.images[0].imageSettings.forceAlphaBlending = true;
	ttSprite.images[0].imageSettings.forceNoWitchGameBending = true;
	ttSprite.images[0].imageSettings.colorTint = vec4f(1.f, 1.f, 1.f, 0.98f);

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
			ttSprite.images[0].imageSettings.colorTint.w = 1.f - clamp01((lifeAccum - fadeStart) / 0.5f);
		}


		lifeAccum += u.dt;
	}
}

RelfAddTypeId(ADecoreGhost, 21'10'09'0001);
ReflBlock() {
	ReflAddActor(ADecoreGhost);
}

} // namespace sge
