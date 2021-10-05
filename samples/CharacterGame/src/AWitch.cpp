#include "imgui/imgui.h"
#include "sge_core/ICore.h"
#include "sge_engine/Actor.h"
#include "sge_engine/GameWorld.h"
#include "sge_engine/actors/AStaticObstacle.h"
#include "sge_engine/traits/TraitCamera.h"
#include "sge_engine/traits/TraitCharacterController.h"
#include "sge_engine/traits/TraitModel.h"
#include "sge_engine/traits/TraitRigidBody.h"
#include "sge_engine/traits/TraitSprite.h"
#include "sge_engine/typelibHelper.h"
#include "sge_utils/math/Random.h"

#include "APumpkinProjectile.h"

namespace sge {

const char* g_decoreAssetNames[] = {
    "assets/decores/fence.png",
    "assets/decores/grassSmall0.png",
    "assets/decores/house1.png",
    "assets/decores/tree1.png",
};

RelfAddTypeIdExists(WitchCameraTrait);
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
RelfAddTypeIdInline(WitchCameraTrait, 21'08'23'0001);

Random g_rnd;

void populateRoad(Actor* road) {
	GameWorld* world = road->getWorld();

	vector_set<ObjectId> children;
	world->getAllChildren(children, road->getId());
	for (ObjectId child : children) {
		world->objectDelete(child);
	}

	const vec3f minRoadPositionWs = road->getPosition();
	const vec3f maxRoadPositionWs = road->getPosition() + vec3f(100.f, 0.f, 0.f);

	const float rangeZMin = 8.f;
	const float rangeZMax = 32.f;

	for (int t = 0; t < 100; t += 10) {
		const int assetIndexPick = g_rnd.nextInt() % SGE_ARRSZ(g_decoreAssetNames);
		std::shared_ptr<Asset> decoreTexAsset =
		    getCore()->getAssetLib()->getAsset(AssetType::Texture2D, g_decoreAssetNames[assetIndexPick], true);
		const float decoreTexWidth = float(decoreTexAsset->asTextureView()->tex->getDesc().texture2D.width);
		const float decoreTexWidthWs = float(decoreTexWidth) / 64.f;

		const float rangeZMinThis = rangeZMin + decoreTexWidthWs * 0.5f;
		const float rangeZMaxThis = rangeZMax - decoreTexWidthWs * 0.5f;

		if (assetIndexPick == 0) {
			const float zSpawnRange = rangeZMaxThis - rangeZMinThis;
			int numFencesToSpawn = int(zSpawnRange / decoreTexWidthWs);

			for (int iFence = 0; iFence < numFencesToSpawn; ++iFence) {
				AStaticObstacle* const decore = world->allocObjectT<AStaticObstacle>();

				float zPosWs = rangeZMinThis + iFence * decoreTexWidthWs;

				decore->m_traitModel.m_models.clear();
				decore->m_traitSprite.m_assetProperty.setAsset(decoreTexAsset);
				decore->m_traitSprite.imageSettings.defaultFacingAxisZ = false;

				decore->setPosition(minRoadPositionWs + vec3f(float(t), 0.f, zPosWs));
				world->setParentOf(decore->getId(), road->getId());
			}

			float zPosWs = g_rnd.nextInRange(rangeZMinThis, rangeZMaxThis);

			zPosWs *= g_rnd.next01() > 0.5f ? 1.f : -1.f;

			
		} else {
			float zPosWs = g_rnd.nextInRange(rangeZMinThis, rangeZMaxThis);

			zPosWs *= g_rnd.next01() > 0.5f ? 1.f : -1.f;

			AStaticObstacle* const decore = world->allocObjectT<AStaticObstacle>();
			decore->m_traitModel.m_models.clear();
			decore->m_traitSprite.m_assetProperty.setAsset(decoreTexAsset);
			decore->m_traitSprite.imageSettings.defaultFacingAxisZ = false;
			decore->m_traitSprite.imageSettings.flipHorizontally = g_rnd.next01() > 0.5f;;

			decore->setPosition(minRoadPositionWs + vec3f(float(t), 0.f, zPosWs));
			world->setParentOf(decore->getId(), road->getId());
		}
	}
}

struct LevelInfo {
	bool isInfoGained = false;
	ObjectId roads[3];

	void gainInfo(GameWorld& world) {
		if (isInfoGained == false) {
			isInfoGained = true;
			roads[0] = ObjectId(1);
			roads[1] = ObjectId(2);
			roads[2] = ObjectId(3);

			populateRoad(world.getActorById(roads[1]));
			populateRoad(world.getActorById(roads[2]));
		}
	}
};



struct AWitch : public Actor {
	TraitRigidBody ttRigidbody;
	TraitSprite ttSprite;
	WitchCameraTrait ttCamera;
	vec3f facingDir = vec3f(1.f, 0.f, 0.f);
	Random m_rnd;

	float rotation = 0.f;
	float stunTimeBecauseOfShooting = 0.f;

	float visualXTilt = 0.f;

	float targetWorldCurvatureY = 0.005f;
	float targetWorldCurvatureZ = 0.04f;
	float timeUntilNextCurvatureTarget = 1.f;

	LevelInfo levelInfo;

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

		getWorld()->worldCurvatureY = targetWorldCurvatureY;
		getWorld()->worldCurvatureZ = targetWorldCurvatureZ;
	}

	void moveLevel(float xDiff) {
		for (ObjectId roadId : levelInfo.roads) {
			if (Actor* a = getWorld()->getActorById(roadId)) {
				vec3f newPos = a->getPosition();
				newPos.x += xDiff;

				if (newPos.x < -100.f) {
					// Regenerate road with new random elements.
					newPos.x += 300.f;

					populateRoad(a);
				}

				a->setPosition(newPos, false);
			}
		}
	}

	void update(const GameUpdateSets& u) {
		ttCamera.update();

		if (u.isGamePaused()) {
			return;
		}

		levelInfo.gainInfo(*getWorld());

		if (timeUntilNextCurvatureTarget <= 0.f) {
			targetWorldCurvatureY = m_rnd.nextInRange(0.001f, 0.003f);
			targetWorldCurvatureZ = m_rnd.nextInRange(-0.08f, 0.08f);
			timeUntilNextCurvatureTarget = 3.f;
		}
		timeUntilNextCurvatureTarget -= u.dt;

		// getWorld()->worldCurvatureY = speedLerp(getWorld()->worldCurvatureY, targetWorldCurvatureY, 0.01f * u.dt);
		getWorld()->worldCurvatureZ = speedLerp(getWorld()->worldCurvatureZ, targetWorldCurvatureZ, 0.1f * u.dt);

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

		vec3f totalPlayerSpeed = inputDirWS * vec3f(20.f, 0.f, 10.f);
		vec3f totalPlayerMovement = inputDirWS * vec3f(20.f, 0.f, 10.f) * u.dt;

		// Move the player in ZY plane
		ttRigidbody.getRigidBody()->setLinearVelocity(totalPlayerSpeed._0yz());

		moveLevel(-totalPlayerMovement.x);


		float tiltAmountSmoothed = smoothstep(fabsf(visualXTilt)) * sign(visualXTilt);

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

RelfAddTypeId(AWitch, 20'03'21'0001);
ReflBlock() {
	ReflAddActor(AWitch);
}

} // namespace sge
