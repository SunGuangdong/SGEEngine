#pragma once

#include "ADecores.h"
#include "AWitch.h"
#include "GlobalRandom.h"
#include "sge_core/ICore.h"
#include "sge_engine/Actor.h"
#include "sge_engine/GameWorld.h"
#include "sge_engine/actors/AStaticObstacle.h"
#include "sge_engine/traits/TraitModel.h"

namespace sge {


const char* g_decoresFences[] = {
    "assets/decores/fence.png",

};

const char* g_decoresTrees[] = {
    "assets/trees/T1.png",
    "assets/trees/T2.png",
    "assets/trees/T3.png",
    "assets/trees/T4.png",
    "assets/trees/T5.png",
    "assets/trees/T6.png",
    "assets/trees/T7.png",
    "assets/trees/T8.png",
    "assets/trees/T9.png",
    "assets/trees/T10.png",
};

const char* g_decoresPumpkins[] = {"assets/pumpkins/P1.png",    "assets/pumpkins/P2.png",     "assets/pumpkins/P3.png",
                                   "assets/pumpkins/P4.png",    "assets/pumpkins/P5.png",     "assets/pumpkins/P6.png",
                                   "assets/pumpkins/P7.png",    "assets/pumpkins/P8.png",     "assets/pumpkins/P9.png",
                                   "assets/pumpkins/P10.png",   "assets/pumpkins/P Set1.png", "assets/pumpkins/P Set2.png",
                                   "assets/pumpkins/P Set3.png"

};

struct ARoadSegment : public Actor {
	ARoadSegment() = default;

	void create() {
		registerTrait(ttModel);

		ttModel.addModel("assets/roadSegment.mdl", true);
	}

	void update(const GameUpdateSets& u) {
		if (u.isGamePaused()) {
			return;
		}

		if (AWitch* witch = getWorld()->getFistObject<AWitch>()) {
			vec3f p = getPosition();
			p.x -= witch->currentSpeedX * u.dt;

			setPosition(p);
		}

		if (getPosition().x < -110.f) {
			vec3f p = getPosition();
			p.x += 300.f;
			setPosition(p);
			generateDecores();
		}
	}

	void generateDecores() {
		GameWorld* world = getWorld();

		// Delete the old decores.
		vector_set<ObjectId> children;
		world->getAllChildren(children, getId());
		for (ObjectId child : children) {
			world->objectDelete(child);
		}

		const float rangeZMin = 8.f;
		const float rangeZMax = 32.f;
		const float rangeZMid = 16.f;

		const vec3f minRoadPositionWs = getPosition();

		for (int t = 0; t < 100; t += 10) {
			const char** assets = nullptr;
			int typePick = g_rnd.nextInt() % 5;
			int maxAssetsForType = 0;
			if (typePick == 0) {
				assets = g_decoresFences;
				maxAssetsForType = SGE_ARRSZ(g_decoresFences);
			} else if (typePick == 1) {
				assets = g_decoresTrees;
				maxAssetsForType = SGE_ARRSZ(g_decoresTrees);
			} else if (typePick == 2) {
				assets = g_decoresPumpkins;
				maxAssetsForType = SGE_ARRSZ(g_decoresPumpkins);
			}else if (typePick == 3 || typePick == 4) {
				assets = g_decoresTrees;
				maxAssetsForType = SGE_ARRSZ(g_decoresTrees);
			}

			const int assetIndexPick = g_rnd.nextInt() % maxAssetsForType;
			std::shared_ptr<Asset> decoreTexAsset = getCore()->getAssetLib()->getAsset(AssetType::Texture2D, assets[assetIndexPick], true);
			const float decoreTexWidth = float(decoreTexAsset->asTextureView()->tex->getDesc().texture2D.width);
			const float decoreTexWidthWs = float(decoreTexWidth) / 64.f;

			const float rangeZMinThis = rangeZMin + decoreTexWidthWs * 0.5f;
			const float rangeZMaxThis = rangeZMax - decoreTexWidthWs * 0.5f;

			if (g_rnd.nextInt() % 100 <= 15) {
				float zPosWs = g_rnd.nextInRange(-rangeZMid, rangeZMid) * g_rnd.nextFlipFLoat();
				ADecoreGhost* const decoreGhost = world->allocObjectT<ADecoreGhost>();

				decoreGhost->setPosition(minRoadPositionWs + vec3f(float(t), -2.f, zPosWs));
				world->setParentOf(decoreGhost->getId(), getId());
			}

			if (typePick == 0) {
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
					world->setParentOf(decore->getId(), getId());
				}

				float zPosWs = g_rnd.nextInRange(rangeZMinThis, rangeZMaxThis);

				zPosWs *= g_rnd.next01() > 0.5f ? 1.f : -1.f;
			} else {
				float zPosWs = g_rnd.nextInRange(rangeZMinThis, rangeZMaxThis);

				zPosWs *= g_rnd.next01() > 0.5f ? 1.f : -1.f;

				AStaticObstacle* const decore = world->allocObjectT<AStaticObstacle>();
				decore->m_traitModel.m_models.clear();
				decore->m_traitSprite.images.resize(1);
				decore->m_traitSprite.images[0].m_assetProperty.setAsset(decoreTexAsset);
				decore->m_traitSprite.images[0].imageSettings.defaultFacingAxisZ = false;
				decore->m_traitSprite.images[0].imageSettings.flipHorizontally = g_rnd.next01() > 0.5f;

				decore->setPosition(minRoadPositionWs + vec3f(float(t), 0.f, zPosWs));

				if (typePick == 2) {
					transf3d trs = decore->getTransform();
					trs.s = vec3f(0.5f);
					decore->setTransform(trs);
				}

				world->setParentOf(decore->getId(), getId());
			}
		}
	}

	virtual AABox3f getBBoxOS() const { return ttModel.getBBoxOS(); }

	TraitModel ttModel;
};

RelfAddTypeId(ARoadSegment, 21'10'03'0001);
ReflBlock() {
	ReflAddActor(ARoadSegment);
}

} // namespace sge
