#include "sge_core/ICore.h"
#include "sge_engine/GameWorld.h"

#include "sge_engine/traits/TraitModel.h"
#include "sge_engine/traits/TraitRigidBody.h"
#include "sge_engine/traits/TraitSprite.h"
#include "sge_engine/typelibHelper.h"

#include "sge_utils/utils/timer.h"

#include "AWitch.h"
#include "GlobalRandom.h"
#include "sge_engine/actors/ALight.h"

namespace sge {

RelfAddTypeId(AWitch, 20'03'21'0001);
ReflBlock() {
	ReflAddActor(AWitch);
}

AABox3f AWitch::getBBoxOS() const {
	return ttSprite.getBBoxOS();
}

void AWitch::applyDamage() {
	if (timeImmune == 0.f) {
		timeImmune = 1.5f;
		health -= 1;
		health = maxOf(health, 0);

		if (!isMobileEmscripten) {
			getCore()->getAudioDevice()->play(&sndHurt, true);
		}
		currentSpeedX = 20.f;
	}
}

void AWitch::create() {
	registerTrait(ttRigidbody);
	registerTrait(ttSprite);

	// Re-seed by time to make each run different.
	g_rnd.setSeed(int(Timer::now_nanoseconds_int()));

	auto imageAsset = getCore()->getAssetLib()->getAsset("assets/witchOnBroom.png", true);
	ttSprite.images.resize(1);
	ttSprite.images[0].m_assetProperty.setAsset(imageAsset);
	ttSprite.images[0].imageSettings.m_anchor = anchor_bottomMid;
	ttSprite.images[0].imageSettings.defaultFacingAxisZ = false;
	ttSprite.images[0].imageSettings.flipHorizontally = true;

	ttRigidbody.getRigidBody()->create(this, CollsionShapeDesc::createCylinderBottomAligned(6.f, 1.f), 1.f, false);
	ttRigidbody.getRigidBody()->setCanRotate(false, false, false);
	ttRigidbody.getRigidBody()->setFriction(0.f);

	getWorld()->worldCurvatureY = targetWorldCurvatureY;
	getWorld()->worldCurvatureZ = targetWorldCurvatureZ;
}

void AWitch::update(const GameUpdateSets& u) {
	if (!areEmscriptenFixesDone) {
		if (isMobileEmscripten) {
			if (Actor* aLight = getWorld()->getActorByName("ALight_4")) {
				ALight* light = (ALight*)aLight;
				light->m_lightDesc.hasShadows = false;
			}
		}

		areEmscriptenFixesDone = true;
	}

	if (u.isGamePaused()) {
		return;
	}



	timeImmune -= u.dt;
	timeImmune = maxOf(timeImmune, 0.f);

	float curvatureZ = targetWorldCurvatureZ;
	float curvatureY = targetWorldCurvatureY;

	if (currentCurvatureRemainingDistance < 300.f) {
		float k = 1.f - currentCurvatureRemainingDistance / 300.f;
		k = k < 0.5 ? 2 * k * k : 1 - powf(-2 * k + 2, 2) / 2;
		curvatureZ = lerp(targetWorldCurvatureZ, nextWorldCurvatureZ, k);
		curvatureY = lerp(targetWorldCurvatureY, nextWorldCurvatureY, k);
	}

	getWorld()->worldCurvatureZ = curvatureZ;
	getWorld()->worldCurvatureY = curvatureY;

	ttRigidbody.getRigidBody()->setGravity(vec3f(0.f));

	// Obtain the input
	vec3f inputDirWS = vec3f(0.f); // The normalized input direction in world space.
	{
		// Keyboard input dir.
		if (u.is.wasActiveWhilePolling() && u.is.AnyArrowKeyDown(true)) {
			inputDirWS.z = u.is.GetArrowKeysDir(false, true).x;
			inputDirWS.y = 0.f;
		}

		// Use gamepad input dif in no keyboard input was used.
		const GamepadState* const gamepad = u.is.getHookedGemepad(0);
		if (inputDirWS == vec3f(0.f) && gamepad) {
			inputDirWS.z = gamepad->getInputDir(true).x;
			inputDirWS.y = 0.f;
		}

		if (u.is.hasTouchPressedUV(vec2f(0.f, 0.f), vec2f(0.5f, 1.f))) {
			inputDirWS.z += -1.f;
		}

		if (u.is.hasTouchPressedUV(vec2f(0.5f, 0.f), vec2f(1.f, 1.f))) {
			inputDirWS.z += 1.f;
		}

		// The charater automatically goes forwards.
		inputDirWS.x = 1.f;
	}

	if (inputDirWS.z == 0.f || sign(inputDirWS.z) != sign(visualXTilt)) {
		visualXTilt = speedLerp(visualXTilt, 0.f, 3.0f * u.dt);
	}

	visualXTilt += 2.f * inputDirWS.z * u.dt;
	visualXTilt = clamp(visualXTilt, -1.f, 1.f);

	if (isDead()) {
		currentSpeedX = speedLerp(currentSpeedX, 0.f, u.dt * 120.f);
	} else {
		currentSpeedX = speedLerp(currentSpeedX, 40.f, u.dt * 80.f);
	}

	currSpeedZ = speedLerp(currSpeedZ, inputDirWS.z * 10.f, u.dt * 110.f);

	const vec3f totalPlayerSpeed = vec3f(currentSpeedX, 0.f, currSpeedZ);

	// Update the curvature of the level.
	currentCurvatureRemainingDistance -= totalPlayerSpeed.x * u.dt;

	if (currentCurvatureRemainingDistance < 0.f) {
		currentCurvatureRemainingDistance = g_rnd.nextInRange(350.f, 500.f) + currentCurvatureRemainingDistance;

		targetWorldCurvatureZ = nextWorldCurvatureZ;
		targetWorldCurvatureY = nextWorldCurvatureY;

		nextWorldCurvatureY = g_rnd.nextInRange(12.f, 18.f);
		nextWorldCurvatureZ = g_rnd.nextInRange(-18.f, 18.f);
	}

	// Move the witch in ZY plane and the rest of the level with her.
	ttRigidbody.getRigidBody()->setLinearVelocity(totalPlayerSpeed._0yz());
	levelInfo.generateInteractables(getWorld(), currentSpeedX * u.dt);

	// Damage blinking.
	vec4f witchTint = vec4f(1.f);
	if (isDamaged()) {
		int k = int(timeImmune / 0.15f);
		if (k % 2 == 0) {
			witchTint = vec4f(1.f, 0.7f, 0.7f, 0.75f);
		} else {
			witchTint = vec4f(1.f, 0.7f, 0.7f, 1.f);
		}
	}

	ttSprite.images[0].imageSettings.colorTint = witchTint;

	// Tilt the witch.
	const mat4f upDownWobble = mat4f::getTranslation(vec3f(0.f, 0.5f + sinf(getWorld()->timeSpendPlaying * 3.14f) * 0.3f, 0.f));
	float tiltAmountSmoothed = fabsf(visualXTilt) * sign(visualXTilt);
	const mat4f tilt = mat4f::getTranslation(0.f, 2.f, 0.f) * mat4f::getRotationX(tiltAmountSmoothed * deg2rad(30.f)) *
	                   mat4f::getTranslation(0.f, -2.f, 0.f);

	ttSprite.images[0].m_additionalTransform = upDownWobble * tilt;

	// Add your logic here.
}



} // namespace sge
