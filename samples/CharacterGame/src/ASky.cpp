#pragma once
#include "sge_core/ICore.h"
#include "sge_engine/Actor.h"
#include "sge_engine/GameWorld.h"
#include "sge_engine/traits/TraitSprite.h"
#include "sge_utils/math/Random.h"

namespace sge {

/// @brief Moon decore.
struct AMoon : public Actor {
	AMoon() = default;

	void create() {
		registerTrait(ttSprite);

		auto assetMoon = getCore()->getAssetLib()->getAsset("assets/sky/Moon.png", true);
		ttSprite.images.resize(1);
		ttSprite.images[0].m_assetProperty.setAsset(assetMoon);
		ttSprite.images[0].imageSettings.defaultFacingAxisZ = false;
		ttSprite.images[0].imageSettings.m_anchor = anchor_mid;
		ttSprite.images[0].imageSettings.forceAlphaBlending = true;
		ttSprite.images[0].imageSettings.forceNoWitchGameBending = true;
		ttSprite.images[0].imageSettings.colorTint = vec4f(1.f, 1.f, 1.f, 0.98f);
	}

	void update(const GameUpdateSets& u) {
		if (u.isGamePaused()) {
			return;
		}

		float zPos = -getWorld()->worldCurvatureZ * 2.0f;
		float yShiftForThisCurvature = (getWorld()->worldCurvatureY - 8.f) * 2.f;
		float yShiftTotal = yShiftForThisCurvature - yShiftBecauseOfCurvature;
		yShiftBecauseOfCurvature = yShiftForThisCurvature;
		vec3f newPos = vec3f(getPosition().x, getPosition().y + yShiftTotal, zPos);
		setPosition(newPos);
	}
	virtual AABox3f getBBoxOS() const { return ttSprite.getBBoxOS(); }

  public:
	TraitSprite ttSprite;
	float yShiftBecauseOfCurvature = 0.f;
};


RelfAddTypeId(AMoon, 21'10'03'0003);
ReflBlock() {
	ReflAddActor(AMoon);
}

const char* cloudTexPaths[] = {
    "assets/sky/C1.png",
    "assets/sky/C2.png",
    "assets/sky/C3.png",
    "assets/sky/C4.png",
};

/// @brief Clouds decore.
struct ACloud : public Actor {
	ACloud() = default;

	void create() {
		registerTrait(ttSprite);

		ttSprite.images.resize(1);
		ttSprite.images[0].imageSettings.defaultFacingAxisZ = false;
		ttSprite.images[0].imageSettings.m_anchor = anchor_mid;
		ttSprite.images[0].imageSettings.forceAlphaBlending = true;
		ttSprite.images[0].imageSettings.forceNoWitchGameBending = true;
		ttSprite.images[0].imageSettings.colorTint = vec4f(1.f, 1.f, 1.f, 0.98f);

		rnd.setSeed(int(size_t(this)));
		randomizeCloud();
	}

	void randomizeCloud() {
		auto assetCloudTex = getCore()->getAssetLib()->getAsset(cloudTexPaths[rnd.nextInt() % SGE_ARRSZ(cloudTexPaths)], true);
		ttSprite.images[0].m_assetProperty.setAsset(assetCloudTex);

		cloudSpeed = rnd.nextInRange(5.f, 11.f);
		cloudSpeed = rnd.nextInt() % 2 ? 1.f : -1.f;

		travelDistanceBeforeReappear = rnd.nextInRange(10.f, 50.f);
		travelDistanceAccum = 0.f;
		baseCloudAlpha = rnd.nextInRange(0.8f, 0.98f);

		vec3f newPos = getPosition();
		newPos.z = rnd.nextInRange(-160.f, 160.f);
		;
		setPosition(newPos);
	}

	void update(const GameUpdateSets& u) {
		if (u.isGamePaused()) {
			return;
		}

		if (travelDistanceAccum >= travelDistanceBeforeReappear) {
			randomizeCloud();
		}

		float alpha = 1.f;
		if (travelDistanceAccum <= 0.1f * travelDistanceBeforeReappear) {
			alpha = travelDistanceAccum / (0.1f * travelDistanceBeforeReappear);
		} else if (travelDistanceAccum >= 0.9f * travelDistanceBeforeReappear) {
			alpha = 1.f - (travelDistanceAccum - 0.9f * travelDistanceBeforeReappear) / (0.1f * travelDistanceBeforeReappear);
		}

		ttSprite.images[0].imageSettings.colorTint = vec4f(1.f, 1.f, 1.f, alpha * baseCloudAlpha);

		// Move the cloud.
		vec3f newPos = getPosition();
		newPos.z = newPos.z + u.dt * cloudSpeed;
		travelDistanceAccum += u.dt * fabsf(cloudSpeed);
		setPosition(newPos);
	}

	virtual AABox3f getBBoxOS() const { return ttSprite.getBBoxOS(); }

  public:
	Random rnd;
	float cloudSpeed = 5.f;
	float travelDistanceBeforeReappear = 600.f;
	float travelDistanceAccum = 0.f;
	float baseCloudAlpha = 1.f;
	TraitSprite ttSprite;
};


RelfAddTypeId(ACloud, 21'10'03'0004);
ReflBlock() {
	ReflAddActor(ACloud);
}

} // namespace sge
