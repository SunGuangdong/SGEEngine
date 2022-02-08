#pragma once

#include "sge_core/Geometry.h"
#include "sge_engine/Actor.h"
#include "sge_engine/GameDrawer/RenderItems/GeometryRenderItem.h"
#include "sge_utils/math/mat4f.h"

namespace sge {

struct IMaterialData;

ReflAddTypeIdExists(TraitRenderGeometry);
/// Represents a custom made geometry that is going to be rendered with
/// the default shaders. (TODO: custom shaders and materials).
struct SGE_ENGINE_API TraitRenderGeometry : public Trait {
	SGE_TraitDecl_Full(TraitRenderGeometry);

	void getRenderItems(DrawReason drawReason, std::vector<GeometryRenderItem>& renderItems);

	/// @brief A Single geometry to be rendered.
	/// This structure doesm't own any pointers, all of them need to be managed manually.
	struct Element {
		Box3f bboxGeometry; ///< The bounding box of the geometry, with no transformations applied (aka the vertex buffer bbox).
		const Geometry* pGeom = nullptr;    ///< The geometry to be rendered. The pointer must be managed manually.
		IMaterialData* pMtl = nullptr;      ///< The material to be used. The pointer must be managed manually.
		mat4f tform = mat4f::getIdentity(); ///< See @isTformInWorldSpace.

		/// If true @tform should be used as if it specified the world space
		/// Ignoring the owning actor transform,
		/// otherwise it is in object space of the owning actor.
		bool isTformInWorldSpace = false;

		/// True if this elements is going to get rendered or not.
		bool isRenderable = true;
	};

	std::vector<Element> geoms;
};

} // namespace sge
