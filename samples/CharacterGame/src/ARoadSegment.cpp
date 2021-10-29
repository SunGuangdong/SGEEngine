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
    "assets/trees/T1.png", "assets/trees/T2.png", "assets/trees/T3.png", "assets/trees/T4.png", "assets/trees/T5.png",
    "assets/trees/T6.png", "assets/trees/T7.png", "assets/trees/T8.png", "assets/trees/T9.png", "assets/trees/T10.png",
};

const char* g_decoresPumpkins[] = {"assets/pumpkins/P1.png",    "assets/pumpkins/P2.png",     "assets/pumpkins/P3.png",
                                   "assets/pumpkins/P4.png",    "assets/pumpkins/P5.png",     "assets/pumpkins/P6.png",
                                   "assets/pumpkins/P7.png",    "assets/pumpkins/P8.png",     "assets/pumpkins/P9.png",
                                   "assets/pumpkins/P10.png",   "assets/pumpkins/P Set1.png", "assets/pumpkins/P Set2.png",
                                   "assets/pumpkins/P Set3.png"

};

const char* g_gravesL[] = {
    "assets/graves/GL1.png", "assets/graves/GL2.png", "assets/graves/GL3.png",
    "assets/graves/GL4.png", "assets/graves/GL5.png", "assets/graves/GL6.png",

};

const char* g_gravesR[] = {
    "assets/graves/GR1.png", "assets/graves/GR2.png", "assets/graves/GR3.png",
    "assets/graves/GR4.png", "assets/graves/GR5.png", "assets/graves/GR6.png",
};

const char* g_lampL = "assets/LampL.png";

std::shared_ptr<Asset> g_lampAsset;


std::shared_ptr<sge::Asset> pickDecoreAsset(const char* assets[], int arrSz) {
	const int elem = g_rnd.nextIntBefore(arrSz);
	return getCore()->getAssetLib()->getAsset(assets[elem], true);
}

enum PickDecoreFlags : int {
	pdf_trees = 1 << 0,
	pdf_pumpkins = 1 << 1,
	pdf_graves = 1 << 2,

	pdf_numFlags = 3,

	pdf_all_mask = 7,
};

std::shared_ptr<sge::Asset> pickDecore(int pdf, bool useLeft, float& outScale) {
	outScale = 1.f;

	int usedCatgoriesNum = 0;
	int possibleCategories[pdf_numFlags] = {0};
	for (int t = 0; t < pdf_numFlags; ++t) {
		if (pdf & (1 << t)) {
			possibleCategories[usedCatgoriesNum] = t;
			usedCatgoriesNum++;
		}
	}

	const int category = possibleCategories[g_rnd.nextIntBefore(usedCatgoriesNum)];

	if (category == 0) {
		outScale = g_rnd.nextInRange(1.4f, 1.8f);
		return pickDecoreAsset(g_decoresTrees, SGE_ARRSZ(g_decoresTrees));
	}
	if (category == 1) {
		return pickDecoreAsset(g_decoresPumpkins, SGE_ARRSZ(g_decoresPumpkins));
	}
	if (category == 2) {
		if (useLeft) {
			return pickDecoreAsset(g_gravesR, SGE_ARRSZ(g_gravesR));
		} else {
			return pickDecoreAsset(g_gravesL, SGE_ARRSZ(g_gravesL));
		}
	}

	sgeAssert(false);
	return std::shared_ptr<sge::Asset>();
}

struct ARoadSegment : public Actor {
	ARoadSegment() = default;

	bool initalDecoresGenerated = false;

	void create() {
		registerTrait(ttModel);

		ttModel.addModel("assets/roadSegment.mdl", true);
		ttModel.forceNoShadows = true;

		if (g_lampAsset == nullptr) {
			g_lampAsset = getCore()->getAssetLib()->getAsset(g_lampL, true);
		}
	}

	void update(const GameUpdateSets& u) {
		if (u.isGamePaused()) {
			return;
		}

		if (!initalDecoresGenerated) {
			generateDecores();
			initalDecoresGenerated = true;
		}

		if (AWitch* witch = getWorld()->getFistObject<AWitch>()) {
			vec3f p = getPosition();
			p.x -= witch->currentSpeedX * u.dt;

			setPosition(p);
		}

		if (getPosition().x < -116.f) {
			vec3f p = getPosition();
			p.x += 108.f * 3.f;
			setPosition(p);
			generateDecores();
		}
	}

	void generateDecores() {
		enum DecoreConfig : int {
			decoreConfig_nothing,
			decoreConfig_2,
			decoreConfig_3,
			decoreConfig_hugeTree,
		};

		// clang-format off
		int cumulativeDecoreConfig[] = {
			decoreConfig_nothing, 130,
		    decoreConfig_2, 170,
		    decoreConfig_3, 170,
			decoreConfig_hugeTree, 50,
		};
		// clang-format on

		int totalCumulativeChance = 0;
		for (int t = 0; t < SGE_ARRSZ(cumulativeDecoreConfig) / 2; t += 2) {
			totalCumulativeChance += cumulativeDecoreConfig[t + 1];
		}

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

		const auto createDecoreActor = [this](const vec3f& posRoadSpace, std::shared_ptr<sge::Asset> asset,
		                                      float visualScale) -> AStaticObstacle* {
			if (asset) {
				AStaticObstacle* const decore = getWorld()->allocObjectT<AStaticObstacle>();
				decore->m_traitModel.m_models.clear();
				decore->m_traitSprite.images.resize(1);
				decore->m_traitSprite.images[0].m_assetProperty.setAsset(std::move(asset));
				decore->m_traitSprite.images[0].imageSettings.defaultFacingAxisZ = false;
				decore->m_traitSprite.images[0].imageSettings.flipHorizontally = true;
				decore->m_traitSprite.images[0].imageSettings.forceAlphaBlending = true;
				decore->m_traitSprite.images[0].m_additionalTransform = mat4f::getScaling(visualScale);

				decore->setPosition(getPosition() + posRoadSpace);
				getWorld()->setParentOf(decore->getId(), getId());
				return decore;
			}
			return nullptr;
		};

		const auto decoratePos = [this, &createDecoreActor](const vec3f& posRoadSpace, int pdf, bool useLeft) {
			float scale = 1.f;
			std::shared_ptr<sge::Asset> asset = pickDecore(pdf, useLeft, scale);
			return createDecoreActor(posRoadSpace, std::move(asset), scale);
		};

		const auto pickDecoreConfig = [&totalCumulativeChance, &cumulativeDecoreConfig]() {
			DecoreConfig decoreConfig = decoreConfig_nothing;
			const int decoreConfigChooser = g_rnd.nextIntBefore(totalCumulativeChance);

			for (int t = 0; t < SGE_ARRSZ(cumulativeDecoreConfig) / 2; t += 2) {
				if (decoreConfigChooser <= cumulativeDecoreConfig[t + 1]) {
					decoreConfig = DecoreConfig(cumulativeDecoreConfig[t]);
				}
			}

			return decoreConfig;
		};

		const auto decorateSide = [&](DecoreConfig decoreConfig, float xPosRoadSpace, bool isLeft) {
			switch (decoreConfig) {
				case decoreConfig_nothing: {
				} break;
				case decoreConfig_2: {
					const float zKeyPos[2] = {16.f, 24.f};
					if (g_rnd.next01() < 0.8f)
						decoratePos(vec3f(xPosRoadSpace + g_rnd.nextInRange(0.f, 1.f), 0.f, zKeyPos[0] * (isLeft ? 1.f : -1.f)),
						            pdf_all_mask, isLeft);
					if (g_rnd.next01() < 0.8f)
						decoratePos(vec3f(xPosRoadSpace + g_rnd.nextInRange(0.f, 1.f), 0.f, zKeyPos[1] * (isLeft ? 1.f : -1.f)),
						            pdf_all_mask, isLeft);

				} break;
				case decoreConfig_3: {
					const float zKeyPos[3] = {12.f, 18.f, 24.f};

					if (g_rnd.next01() < 0.8f)
						decoratePos(vec3f(xPosRoadSpace + g_rnd.nextInRange(0.f, 1.f), 0.f, zKeyPos[0] * (isLeft ? 1.f : -1.f)), pdf_graves,
						            isLeft);
					if (g_rnd.next01() < 0.8f)
						decoratePos(vec3f(xPosRoadSpace + g_rnd.nextInRange(0.f, 1.f), 0.f, zKeyPos[1] * (isLeft ? 1.f : -1.f)),
						            pdf_trees | pdf_graves, isLeft);
					if (g_rnd.next01() < 0.8f)
						decoratePos(vec3f(xPosRoadSpace + g_rnd.nextInRange(0.f, 1.f), 0.f, zKeyPos[2] * (isLeft ? 1.f : -1.f)),
						            pdf_trees | pdf_graves, isLeft);

				} break;
				case decoreConfig_hugeTree: {
					float zKeyPos = 20.f;
					decoratePos(vec3f(xPosRoadSpace + g_rnd.nextInRange(0.f, 1.f), 0.f, zKeyPos * (isLeft ? 1.f : -1.f)),
					            pdf_trees | pdf_graves, isLeft)
					    ->m_traitSprite.additionalTransform = mat4f::getScaling(2.f);
				} break;
			}
		};

		for (int t = 0; t < 108; t += 1) {
			// Pick the configuration of the decores.
			if (t % 10 == 0) {
				DecoreConfig decoreConfigLeft = pickDecoreConfig();
				DecoreConfig decoreConfigRight = pickDecoreConfig();

				decorateSide(decoreConfigLeft, float(t), true);
				decorateSide(decoreConfigRight, float(t), false);
			}

			// Add the light on the side walk.
			if (t == 50) {
				createDecoreActor(vec3f(float(t), 0, rangeZMin), g_lampAsset, 1.f)->m_traitSprite.images[0].imageSettings.flipHorizontally =
				    false;
			}

			if (t == 100) {
				createDecoreActor(vec3f(float(t), 0, -rangeZMin), g_lampAsset, 1.f)
				    ->m_traitSprite.images[0]
				    .imageSettings.flipHorizontally = true;
			}

			// Trees.
			if (t % 15 == 0) {
				if (g_rnd.nextBool())
					decoratePos(vec3f(float(t), 0.f, -rangeZMax), pdf_trees, false)->m_traitSprite.additionalTransform =
					    mat4f::getScaling(g_rnd.nextInRange(1.86f, 2.1f));
				if (g_rnd.nextBool())
					decoratePos(vec3f(float(t), 0.f, rangeZMax), pdf_trees, false)->m_traitSprite.additionalTransform =
					    mat4f::getScaling(g_rnd.nextInRange(1.86f, 2.1f));
			}

			// Flying ghost decores.
			if ((t % 10 == 0) && g_rnd.nextInt() % 100 < 15) {
				float zPosWs = g_rnd.nextInRange(-rangeZMid, rangeZMid) * g_rnd.nextFlipFLoat();
				ADecoreGhost* const decoreGhost = world->allocObjectT<ADecoreGhost>();

				decoreGhost->setPosition(minRoadPositionWs + vec3f(float(t) + 0.03f, -2.f, zPosWs));
				world->setParentOf(decoreGhost->getId(), getId());
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
