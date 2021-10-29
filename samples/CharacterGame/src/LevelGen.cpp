
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

void LevelInfo::generateInteractables(GameWorld* world, float travelDistnaceThisFrame) {
	totalGeneratedDistance -= travelDistnaceThisFrame;

	if (totalGeneratedDistance > 200) {
		return;
	}

	const float rangeZMin = 8.f;
	const float rangeZMid = 16.f;

	Rangef onRoadRange = Rangef(-8.f, 8.f);

	float distanceToReach = totalGeneratedDistance + 200.f;

	float diffucultySpacing = 20.f;

	for (int iIter = 0; iIter < 1000; ++iIter) {
		while (totalGeneratedDistance < distanceToReach) {
			const float rndChoice = g_rnd.next01();
			if (rndChoice < 0.15f) {
				// Single bat.
				float zPosWs = g_rnd.nextInRange(-rangeZMin, rangeZMin) * g_rnd.nextFlipFLoat();
				ABat* const bat = world->allocObjectT<ABat>();

				bat->setPosition(vec3f(totalGeneratedDistance, 0.f, zPosWs));
				totalGeneratedDistance += 10.f + diffucultySpacing;
			} else if (rndChoice < 0.25f) {
				// Ghost circle.
				float zPosWs = g_rnd.nextFlipFLoat() * rangeZMid * g_rnd.nextFlipFLoat();
				AGhostCircle* const enemy = world->allocObjectT<AGhostCircle>();

				if (g_rnd.nextBool()) {
					Actor* const candy = world->allocActor(sgeTypeId(ACandy));
					candy->setPosition(vec3f(totalGeneratedDistance, 0.f, -3.f));

					Actor* const candy3 = world->allocActor(sgeTypeId(ACandy));
					candy3->setPosition(vec3f(totalGeneratedDistance, 0.f, 3.f));
				}

				enemy->setPosition(vec3f(totalGeneratedDistance, 0.f, zPosWs));
				totalGeneratedDistance += 15.f + g_rnd.next01() * 10.f;

			} else if (rndChoice < 0.35f) {
				// Bat chain.
				float zPosWs = g_rnd.nextInRange(-rangeZMin, rangeZMin) * g_rnd.nextFlipFLoat();

				bool flipZPos = zPosWs > 1.f && zPosWs < 4.f;

				int numChains = g_rnd.nextInt() % 4;
				for (int iChain = 0; iChain < numChains; ++iChain) {
					for (int t = 0; t < 4; ++t) {
						ABat* const bat = world->allocObjectT<ABat>();

						bat->setPosition(vec3f(totalGeneratedDistance, 0.f, zPosWs + g_rnd.nextInRange(-0.1f, 0.1f)));
						totalGeneratedDistance += 6.f;
					}

					totalGeneratedDistance += 6.f;
					int numPostCandies = 3 + g_rnd.nextIntBefore(5);
					for (int t = 0; t < numPostCandies; ++t) {
						Actor* const candy = world->allocActor(sgeTypeId(ACandy));
						candy->setPosition(vec3f(totalGeneratedDistance, 0.f, zPosWs + g_rnd.nextInRange(-0.1f, 0.1f)));

						totalGeneratedDistance += 4.f;
					}

					if (flipZPos)
						zPosWs *= -1.f;
				}

				totalGeneratedDistance += 20.f + diffucultySpacing;


			} else if (rndChoice < 0.37f) {
				// Strip of nothing or candies, or candies with bats
				float emptyStripLen = 21.f + float(g_rnd.nextInt() % 5) * 21.f;

				float flipCosine = g_rnd.nextFlipFLoat();
				bool hasBats = g_rnd.next01() < 0.3333f;

				if (g_rnd.next01() < 0.8f) {
					for (int t = 0; t < int(emptyStripLen); t += 7) {
						const float cosVal = cos((float(t) / 42.f) * two_pi()) * flipCosine;
						float zPosWs = cosVal * 4.f;

						if (hasBats && ((t / 7) % 3 == 0)) {
							ABat* const bat = world->allocObjectT<ABat>();
							bat->setPosition(vec3f(totalGeneratedDistance, 0.f, -cosVal * 3.75f));
						}

						Actor* const candy = world->allocActor(sgeTypeId(ACandy));
						candy->setPosition(vec3f(totalGeneratedDistance, 0.f, zPosWs));

						totalGeneratedDistance += 4.f;
					}

					totalGeneratedDistance += 10 + diffucultySpacing;
				} else {
					totalGeneratedDistance += emptyStripLen;
				}
			} else if (rndChoice < 0.45f) {
				float zPosWs = g_rnd.nextInRange(-rangeZMin, rangeZMin);
				APumpkinOnRoad* const bat = world->allocObjectT<APumpkinOnRoad>();

				bat->setPosition(vec3f(totalGeneratedDistance, 0.f, zPosWs));
				totalGeneratedDistance += 20.f + diffucultySpacing;
			} else if (rndChoice < 0.50f) {
				// Dual pumpkin on a side.
				APumpkinOnRoad* const p0 = world->allocObjectT<APumpkinOnRoad>();
				p0->setPosition(vec3f(totalGeneratedDistance, 0.f, 6.f));

				APumpkinOnRoad* const p1 = world->allocObjectT<APumpkinOnRoad>();
				p1->setPosition(vec3f(totalGeneratedDistance, 0.f, -6.f));

				Actor* const candy = world->allocActor(sgeTypeId(ACandy));
				candy->setPosition(vec3f(totalGeneratedDistance, 0.f, 0.f));

				totalGeneratedDistance += 20.f + diffucultySpacing;
			} else if (rndChoice < 0.60f) {
				// Left Side Ghost Single.
				ALeftRightGhost* lrGhost = world->allocObjectT<ALeftRightGhost>();
				lrGhost->setPosition(vec3f(totalGeneratedDistance, 0.f, 8.f * g_rnd.nextFlipFLoat()));

				totalGeneratedDistance += 15.f + diffucultySpacing;
			} else if (rndChoice < 0.70f) {
				totalGeneratedDistance += 10.f;
				// Left Side Ghosts Multiple.


				bool hasCandies = g_rnd.next01() <= 0.25f;

				int numGhosts = 3;
				float flipSide = g_rnd.nextFlipFLoat();
				for (int t = 0; t < numGhosts; ++t) {
					ALeftRightGhost* lrGhost = world->allocObjectT<ALeftRightGhost>();
					lrGhost->setPosition(vec3f(totalGeneratedDistance, 0.f, 8.f * flipSide));
					flipSide *= -1.f;

					if (hasCandies) {
						Actor* const candy = world->allocActor(sgeTypeId(ACandy));
						candy->setPosition(vec3f(totalGeneratedDistance + 3.f, 0.f, 0.f));
					}

					totalGeneratedDistance += 6.f;
				}

				totalGeneratedDistance += 20.f + diffucultySpacing;
			}
		}
	}
}

} // namespace sge
