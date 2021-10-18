
#include "sge_core/ICore.h"
#include "sge_engine/GameWorld.h"
#include "sge_engine/actors/AStaticObstacle.h"
#include "sge_utils/math/Rangef.h"

#include "ABat.h"
#include "ADecores.h"
#include "AGhostCircle.h"
#include "GlobalRandom.h"
#include "LevelGen.h"

namespace sge {

struct ACandy;

const char* g_decoreAssetNames[] = {
    "assets/decores/fence.png",
    "assets/pumpkins/P1.png",
    "assets/pumpkins/P2.png",
    "assets/pumpkins/P3.png",
    "assets/pumpkins/P4.png",
    "assets/pumpkins/P5.png",
    "assets/pumpkins/P6.png",
    "assets/pumpkins/P7.png",
    "assets/pumpkins/P8.png",
    "assets/pumpkins/P9.png",
    "assets/pumpkins/P10.png",
    "assets/pumpkins/P Set1.png",
    "assets/pumpkins/P Set2.png",
    "assets/pumpkins/P Set3.png",
    "assets/decores/tree1.png",
};

void LevelInfo::gainInfo(GameWorld& world) {
	if (isInfoGained == false) {
		isInfoGained = true;
		roads[0] = ObjectId(1);
		roads[1] = ObjectId(2);
		roads[2] = ObjectId(3);

		populateRoad(world.getActorById(roads[1]));
		populateRoad(world.getActorById(roads[2]));
	}
}

void LevelInfo::moveLevel(GameWorld* world, float xDiff) {
	for (const ObjectId roadId : roads) {
		if (Actor* const road = world->getActorById(roadId)) {
			vec3f newPos = road->getPosition();
			newPos.x += xDiff;

			// If the road is behind view, move it to the back and regenerate it.
			if (newPos.x < -110.f) {
				newPos.x += 300.f;
				populateRoad(road);
			}

			road->setPosition(newPos, false);
		}
	}
}
inline void LevelInfo::populateRoad(Actor* road) {
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
	const float rangeZMid = 16.f;

	Rangef onRoadRange = Rangef(-8.f, 8.f);

	float enemiesFilledDistance = 0.f;

	for (int iIter = 0; iIter < 1000; ++iIter) {
		while (enemiesFilledDistance < 99.f) {
			const float rndChoice = rnd.next01();
			if (rndChoice < 0.10f) {
				float zPosWs = rnd.nextInRange(-rangeZMin, rangeZMin) * rnd.nextFlipFLoat();
				ABat* const bat = world->allocObjectT<ABat>();

				bat->setPosition(minRoadPositionWs + vec3f(enemiesFilledDistance, 0.f, zPosWs));
				world->setParentOf(bat->getId(), road->getId());

				enemiesFilledDistance += 20.f;
			} else if (rndChoice < 0.20f) {
				float zPosWs = rnd.nextFlipFLoat() * rangeZMid * rnd.nextFlipFLoat();
				AGhostCircle* const enemy = world->allocObjectT<AGhostCircle>();

				enemy->setPosition(minRoadPositionWs + vec3f(enemiesFilledDistance, 0.f, zPosWs));
				world->setParentOf(enemy->getId(), road->getId());

				enemiesFilledDistance += 40.f;
			} else if (rndChoice < 0.30f) {
				// Bat chain.
				float zPosWs = rnd.nextInRange(-rangeZMin, rangeZMin) * rnd.nextFlipFLoat();
				for (int t = 0; t < 4; ++t) {
					ABat* const bat = world->allocObjectT<ABat>();

					bat->setPosition(minRoadPositionWs + vec3f(enemiesFilledDistance, 0.f, zPosWs + rnd.nextInRange(-0.1f, 0.1f)));
					world->setParentOf(bat->getId(), road->getId());

					enemiesFilledDistance += 6.f;
				}

				enemiesFilledDistance += 6.f;
				int numPostCandies = 3 + g_rnd.nextIntBefore(5);
				for (int t = 0; t < numPostCandies; ++t) {
					Actor* const candy = world->allocActor(sgeTypeId(ACandy));
					candy->setPosition(minRoadPositionWs + vec3f(enemiesFilledDistance, 0.f, zPosWs + rnd.nextInRange(-0.1f, 0.1f)));
					world->setParentOf(candy->getId(), road->getId());

					enemiesFilledDistance += 4.f;
				}
			} else if (rndChoice < 0.50f) {
				float emptyStripLen = 10 + g_rnd.next01() * 30.f;

				if (rnd.next01() < 0.3f) {
					int numCandiesToSpawn = int(emptyStripLen / 4.f);
					for (int t = 0; t < numCandiesToSpawn; ++t) {
						float zPosWs = sin(((t * 4.f) / emptyStripLen) * two_pi() ) * 4.f;

						Actor* const candy = world->allocActor(sgeTypeId(ACandy));
						candy->setPosition(minRoadPositionWs + vec3f(enemiesFilledDistance, 0.f, zPosWs));
						world->setParentOf(candy->getId(), road->getId());

						enemiesFilledDistance += 4.f;
					}
				} else {
					enemiesFilledDistance += emptyStripLen;
				}
			}
		}
	}

	for (int t = 0; t < 100; t += 10) {
		const int assetIndexPick = rnd.nextInt() % SGE_ARRSZ(g_decoreAssetNames);
		std::shared_ptr<Asset> decoreTexAsset =
		    getCore()->getAssetLib()->getAsset(AssetType::Texture2D, g_decoreAssetNames[assetIndexPick], true);
		const float decoreTexWidth = float(decoreTexAsset->asTextureView()->tex->getDesc().texture2D.width);
		const float decoreTexWidthWs = float(decoreTexWidth) / 64.f;

		const float rangeZMinThis = rangeZMin + decoreTexWidthWs * 0.5f;
		const float rangeZMaxThis = rangeZMax - decoreTexWidthWs * 0.5f;

		if (rnd.nextInt() % 100 <= 15) {
			float zPosWs = rnd.nextInRange(-rangeZMid, rangeZMid) * rnd.nextFlipFLoat();
			ADecoreGhost* const decoreGhost = world->allocObjectT<ADecoreGhost>();

			decoreGhost->setPosition(minRoadPositionWs + vec3f(float(t), -2.f, zPosWs));
			world->setParentOf(decoreGhost->getId(), road->getId());
		}

		if (assetIndexPick == 0) {
			const float zSpawnRange = rangeZMaxThis - rangeZMinThis;
			int numFencesToSpawn = int(zSpawnRange / decoreTexWidthWs);

			for (int iFence = 0; iFence < numFencesToSpawn; ++iFence) {
				AStaticObstacle* const decore = world->allocObjectT<AStaticObstacle>();

				float zPosWs = rangeZMinThis + iFence * decoreTexWidthWs;

				decore->m_traitModel.m_models.clear();
				decore->m_traitSprite.images.resize(1);
				decore->m_traitSprite.images[0].m_assetProperty.setAsset(decoreTexAsset);
				decore->m_traitSprite.images[0].imageSettings.defaultFacingAxisZ = false;

				decore->setPosition(minRoadPositionWs + vec3f(float(t), 0.f, zPosWs));
				world->setParentOf(decore->getId(), road->getId());
			}

			float zPosWs = rnd.nextInRange(rangeZMinThis, rangeZMaxThis);

			zPosWs *= rnd.next01() > 0.5f ? 1.f : -1.f;
		} else {
			float zPosWs = rnd.nextInRange(rangeZMinThis, rangeZMaxThis);

			zPosWs *= rnd.next01() > 0.5f ? 1.f : -1.f;

			AStaticObstacle* const decore = world->allocObjectT<AStaticObstacle>();
			decore->m_traitModel.m_models.clear();
			decore->m_traitSprite.images.resize(1);
			decore->m_traitSprite.images[0].m_assetProperty.setAsset(decoreTexAsset);
			decore->m_traitSprite.images[0].imageSettings.defaultFacingAxisZ = false;
			decore->m_traitSprite.images[0].imageSettings.flipHorizontally = rnd.next01() > 0.5f;

			decore->setPosition(minRoadPositionWs + vec3f(float(t), 0.f, zPosWs));
			world->setParentOf(decore->getId(), road->getId());
		}
	}
}
} // namespace sge
