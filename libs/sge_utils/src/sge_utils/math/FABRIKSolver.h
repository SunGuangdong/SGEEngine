#pragma once
#include "sge_utils/math/vec3f.h"

namespace sge {

/// Solves an IK chain using the FABRIK alogorithm. the input and output is in @inoutPoints.
/// @param [in] numPoints the number of points in inoutPoints. The Minimum is 2.
/// @param [in/out] inoutPoints the input and the output points after resolving the IK.
/// @param [in] linkLengths is the length between all consequtive points, the size must be @numPoints - 1.
/// @param [in] endAffector is the point where the end point should strive to be located at.
/// @param [in] pPole is an optional value (may be null). The point is in the same space as @inoutPoints and defines
/// direction where each join should point at after being solved - For example, in a leg, this is the point where you
/// want the knee to point at.
/// @param [in] maxIterations the maxium amount of interations that the solver should perform.
/// @param [in] earlyExitDelta if last point is within this distance to the @endAffector then the algorithm will early
/// exit.
void FABRIKSolver(
    const int numPoints,
    vec3f inoutPoints[],
    const float linkLengths[],
    const vec3f& endAffector,
    const vec3f* pPole,
    int maxIterations = 3,
    float earlyExitDelta = 1e-3f);

} // namespace sge
