#include "imgui/imgui.h"
#include "sge_core/ICore.h"
#include "sge_engine/Actor.h"
#include "sge_engine/GameWorld.h"
#include "sge_engine/traits/TraitCamera.h"
#include "sge_engine/traits/TraitCharacterController.h"
#include "sge_engine/traits/TraitModel.h"
#include "sge_engine/traits/TraitRigidBody.h"
#include "sge_engine/traits/TraitSprite.h"
#include "sge_engine/typelibHelper.h"

#include "APumpkinProjectile.h"

namespace sge {

DefineTypeIdExists(WitchCameraTrait);
struct WitchCameraTrait : TraitCamera {
	SGE_TraitDecl_Final(WitchCameraTrait);

	float camPosX = 0.f;

	RawCamera rawCamera;

	WitchCameraTrait() {}

	void update() {
		const vec3f camPos = vec3f(getActor()->getPosition().x - 22.f, 11.0f, 0.f);
		const vec3f lookDir = vec3f(1.f, 0.f, 0.f);
		const vec3f up = vec3f(0.f, 1.f, 0.f);

		const mat4f view = mat4f::getLookAtRH(camPos, camPos + lookDir, up);
		const mat4f proj = mat4f::getPerspectiveFovRH(deg2rad(40.f), getWorld()->userProjectionSettings.aspectRatio, 1.f, 1000.f, -0.120f,
		                                              kIsTexcoordStyleD3D);

		rawCamera = RawCamera(camPos, view, proj);
	}

	virtual ICamera* getCamera() override { return &rawCamera; }
	virtual const ICamera* getCamera() const override { return &rawCamera; }
};
DefineTypeIdInline(WitchCameraTrait, 21'08'23'0001);

struct AWitch : public Actor {
	TraitRigidBody ttRigidbody;
	TraitSprite ttSprite;
	WitchCameraTrait ttCamera;
	vec3f facingDir = vec3f(1.f, 0.f, 0.f);

	float rotation = 0.f;
	float stunTimeBecauseOfShooting = 0.f;

	float visualXTilt = 0.f;

	virtual AABox3f getBBoxOS() const { return AABox3f(); }

	void create() {
		registerTrait(ttRigidbody);
		registerTrait(ttSprite);
		registerTrait(ttCamera);

		ttCamera.update();

		auto imageAsset = getCore()->getAssetLib()->getAsset("assets/witchOnBroom.png", true);
		ttSprite.m_assetProperty.setAsset(imageAsset);
		ttSprite.imageSettings.m_anchor = anchor_mid;
		ttSprite.imageSettings.defaultFacingAxisZ = false;

		ttRigidbody.getRigidBody()->create(this, CollsionShapeDesc::createSphere(1.f, transf3d(vec3f(0.f, 0.5f, 0.f))), 1.f, false);
		ttRigidbody.getRigidBody()->setCanRotate(false, false, false);
		ttRigidbody.getRigidBody()->setFriction(0.f);
	}

	void update(const GameUpdateSets& u) {
		ttCamera.update();

		if (u.isGamePaused()) {
			return;
		}

		ttRigidbody.getRigidBody()->setGravity(vec3f(0.f));

		stunTimeBecauseOfShooting -= u.dt;
		stunTimeBecauseOfShooting = maxOf(0.f, stunTimeBecauseOfShooting);

		bool isShootButtonPressed = u.is.IsKeyPressed(Key_C);
		if (isShootButtonPressed) {
			APumpkinProjectile* proj = getWorld()->allocObjectT<APumpkinProjectile>();
			if (proj) {
				proj->setPosition(getPosition() + facingDir + vec3f(0.f, 2.50f, 0.f));
				proj->horizontalFlightDir = vec3f(1.f, 0.f, 0.f);
			}

			stunTimeBecauseOfShooting = 0.15f;
		}

		// Obtain the input
		const GamepadState* const gamepad = u.is.getHookedGemepad(0);

		vec3f inputDirWS = vec3f(0.f); // The normalized input direction in world space.

		{
			vec3f inputDir = vec3f(0.f);

			// Keyboard input dir.
			if (u.is.wasActiveWhilePolling() && u.is.AnyArrowKeyDown(true)) {
				inputDir = vec3f(u.is.GetArrowKeysDir(true, true), 0);
				inputDir.z = inputDir.x;
				inputDir.x = inputDir.y;
				inputDir.y = 0.f;
			}

			// Use gamepad input dif in no keyboard input was used.
			if (inputDir == vec3f(0.f) && gamepad) {
				inputDir.x = -inputDir.x;
				inputDir = vec3f(gamepad->getInputDir(true), 0.f);
				inputDir.z = inputDir.y; // Remap from gamepad space to camera-ish space.
				inputDir.y = 0.f;
			}

			// Correct the input so it is aligned with the camera.
			inputDirWS = inputDir;
		}

		if (fabsf(inputDirWS.x) > 1e-3f) {
			facingDir = vec3f(inputDirWS.x, 0.f, 0.f).normalized0();
		}


		if (inputDirWS.z == 0.f)
			visualXTilt = speedLerp(visualXTilt, 0.f, 3.0f * u.dt);

		visualXTilt += inputDirWS.z * u.dt * 2.f;
		visualXTilt = clamp(visualXTilt, -1.f, 1.f);

		float tiltAmountSmoothed =  smoothstep(fabsf(visualXTilt)) * sign(visualXTilt);

		ttRigidbody.getRigidBody()->setLinearVelocity(inputDirWS * vec3f(20.f, 0.f, 10.f));

		ttSprite.m_additionalTransform =
		    mat4f::getTranslation(vec3f(0.f, 1.f, 0.f) * sinf(getWorld()->timeSpendPlaying * 3.14f * 0.5f) * 0.2f) *
		    mat4f::getRotationX(tiltAmountSmoothed * deg2rad(30.f));

		// Add your logic here.
	}

	void postUpdate(const GameUpdateSets& u) {
		if (u.isGamePaused()) {
			return;
		}

		// Add your logic here.
	}
};

DefineTypeId(AWitch, 20'03'21'0001);
ReflBlock() {
	ReflAddActor(AWitch);
}

} // namespace sge
