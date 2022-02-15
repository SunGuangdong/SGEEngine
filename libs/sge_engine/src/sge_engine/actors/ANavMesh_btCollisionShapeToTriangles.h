#pragma once

#include "sge_engine/physics/CollisionShape.h"
#include "sge_engine/sge_engine_api.h"

namespace sge {

/// Generates triangles lists in world space, representing the collision shape.
/// @param[out] outVertices holds the resulting vertices needed to represent the body.
/// @param[out] outIndices holds the resulting indices pointing at vertices that produce the triangle lists.
SGE_ENGINE_API void bulletCollisionShapeToTriangles(
    const btCollisionShape* const collisionShape,
    const btTransform& parentTransform,
    std::vector<vec3f>& outVertices,
    std::vector<int>& outIndices);

} // namespace sge
