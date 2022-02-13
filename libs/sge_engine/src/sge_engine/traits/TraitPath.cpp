#pragma once

#include "TraitPath.h"
#include "sge_engine/GameWorld.h"

namespace sge {

ReflAddTypeId(BounceType, 20'03'02'0029);
// clang-format off

ReflBlock() {
	ReflAddType(BounceType)
		ReflEnumVal(bounceType_bounce, "bounceType_bounce")
		ReflEnumVal(bouceType_reset, "bouceType_reset")
		ReflEnumVal(bounceType_stop, "bounceType_stop")
		ReflEnumVal(bounceType_onForwardOffBackwards, "bounceType_onForwardOffBackwards")
		ReflEnumVal(bounceType_idle, "bounceType_idle")
	;
}
// clang-format on

//-----------------------------------------------------------------------
//
//-----------------------------------------------------------------------
ReflAddTypeId(PathLengthFollow::Settings, 20'03'02'0030);
// clang-format off

ReflBlock() {
	ReflAddType(PathLengthFollow::Settings)
		ReflMember(PathLengthFollow::Settings, isReversed)
		ReflMember(PathLengthFollow::Settings, bounceType)
		ReflMember(PathLengthFollow::Settings, speed)
		ReflMember(PathLengthFollow::Settings, restingTime)
	;
}
// clang-format on

PathLengthFollow::State PathLengthFollow::compute(const float pathLength,
                                                  const float dt,
                                                  GameWorld* const world,
                                                  const Settings& settings,
                                                  const State& prevState,
                                                  const int recursionDepth)
{
	const int kMaxRecursions = 3;

	State result = prevState;

	const bool shouldUpdate = dt > 1e-6f;

	bool isResting = false;
	float time = dt;

	if (prevState.timeLeftToRest > 0.f) {
		result.timeLeftToRest = prevState.timeLeftToRest - time;
		if (result.timeLeftToRest < 0.f) {
			time = maxOf(-result.timeLeftToRest, 0.f);
			result.timeLeftToRest = 0.f;
		}
		else {
			isResting = true;
			time = 0.f;
		}
	}

	if (isResting == false) {
		const bool speedIsPositive = prevState.speedIsPositive;

		float maxOffsetThisStep = time * settings.speed * (speedIsPositive ? 1.f : -1.f);
		time = 0.f;
		float newTravelDistanceThisStep = prevState.distanceAlongPath + maxOffsetThisStep;

		bool pathEndReached = false;

		if (newTravelDistanceThisStep <= 0.f) {
			maxOffsetThisStep -= newTravelDistanceThisStep - 0.f;
			time = (0.f - newTravelDistanceThisStep) / settings.speed;
			newTravelDistanceThisStep = 0.f;
			pathEndReached = true;
		}

		if (newTravelDistanceThisStep >= pathLength) {
			maxOffsetThisStep -= newTravelDistanceThisStep - pathLength;
			time = (newTravelDistanceThisStep - pathLength) / settings.speed;
			newTravelDistanceThisStep = pathLength;
			pathEndReached = true;
		}

		// The time now hold the time after the follower moved.
		// maxOffsetThisStep holds the movement.

		switch (settings.bounceType) {
			case bounceType_bounce: {
				if (pathEndReached && shouldUpdate) {
					result.speedIsPositive = !speedIsPositive;
				}

				result.distanceAlongPath = newTravelDistanceThisStep;
			} break;
			case bouceType_reset: {
				result.distanceAlongPath = newTravelDistanceThisStep;
				if (pathEndReached && shouldUpdate) {
					result.distanceAlongPath = fabsf(newTravelDistanceThisStep - pathLength);
				}

			} break;
			case bounceType_stop: {
				result.distanceAlongPath = clamp(newTravelDistanceThisStep, 0.f, pathLength);
				time = 0.f;
			} break;

			case bounceType_idle: {
				result.distanceAlongPath = clamp(result.distanceAlongPath, 0.f, pathLength);
			} break;

			default:
				sgeAssert(false && "BounceType missign implementation");
				time = 0.f;
		}

		if (pathEndReached) {
			result.timeLeftToRest = settings.restingTime;
		}
	}

	if (time > 1e-6f && kMaxRecursions < 3) {
		return compute(pathLength, time, world, settings, result, recursionDepth + 1);
	}

	// No recurson needed.
	const float evalDistance = settings.isReversed ? pathLength - result.distanceAlongPath : result.distanceAlongPath;
	result.evalDistanceAlongPath = evalDistance;

	sgeAssert(recursionDepth < kMaxRecursions && "Probably there is a mistake in this function");
	return result;
}

//-----------------------------------------------------------------------
//
//-----------------------------------------------------------------------
ReflAddTypeId(PathFollow::Settings, 20'03'02'0031);
ReflAddTypeId(PathFollow::State, 20'03'02'0033);
// clang-format off

ReflBlock() {
	ReflAddType(PathFollow::Settings)
		ReflMember(PathFollow::Settings, pathId)
		ReflMember(PathFollow::Settings, isReversed)
		ReflMember(PathFollow::Settings, bounceType)
		ReflMember(PathFollow::Settings, speed)
		ReflMember(PathFollow::Settings, restingTime)
	;

	ReflAddType(PathFollow::State)
		ReflMember(PathFollow::State, distanceAlongPath)
		ReflMember(PathFollow::State, timeLeftToRest)
		ReflMember(PathFollow::State, evalDistanceAlongPath)
		ReflMember(PathFollow::State, speedIsPositive)
		ReflMember(PathFollow::State, ptWs)
	;
}
// clang-format on

Optional<PathFollow::State> PathFollow::compute(const float dt, GameWorld* const world, const Settings& settings, const State& prevState)
{
	TraitPath3D* const path = getTrait<TraitPath3D>(world->getActorById(settings.pathId));

	if (path == nullptr || path->isEmpty()) {
		return NullOptional();
	}

	PathLengthFollow::Settings plSettings = {settings.isReversed, settings.speed, settings.restingTime, settings.bounceType};

	PathLengthFollow::State plPrevState = {
	    prevState.timeLeftToRest,
	    prevState.distanceAlongPath,
	    prevState.evalDistanceAlongPath,
	    prevState.speedIsPositive,
	};

	const PathLengthFollow::State newState = PathLengthFollow::compute(path->getTotalLength(), dt, world, plSettings, plPrevState);

	State resultState = {newState.timeLeftToRest, newState.distanceAlongPath, newState.evalDistanceAlongPath, newState.speedIsPositive,
	                     vec3f(0.f)};

	if (path->evaluateAtDistance(&resultState.ptWs, nullptr, resultState.evalDistanceAlongPath)) {
		mat4f const logicTransformMtx = path->getActor()->getTransformMtx();
		resultState.ptWs = mat_mul_pos(logicTransformMtx, resultState.ptWs);
	}
	else {
		return NullOptional();
	}

	return resultState;
}


float computePathLength(const std::vector<vec3f>& path)
{
	float pathLength = 0.f;
	for (int t = 1; t < int(path.size()); ++t) {
		pathLength += distance(path[t], path[t - 1]);
	}

	return pathLength;
}

vec3f samplePathLazy(const std::vector<vec3f>& path, float distance)
{
	for (int t = 1; t < int(path.size()); ++t) {
		float segmentLen = path[t].distance(path[t - 1]);

		if (distance > segmentLen) {
			distance -= segmentLen;
		}
		else {
			float k = distance / segmentLen;
			return lerp(path[t - 1], path[t], k);
		}
	}

	return path.back();
}

} // namespace sge
