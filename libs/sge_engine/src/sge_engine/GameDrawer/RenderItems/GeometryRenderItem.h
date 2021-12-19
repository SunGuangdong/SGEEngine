#pragma once

#include "sge_engine/GameDrawer/IRenderItem.h"
#include "sge_utils/math/Box.h"
#include "sge_utils/math/mat4.h"

namespace sge {

struct IMaterialData;

/// A render item representing a geometry to get drawn
/// with a specfied material at a specified location.
struct GeometryRenderItem : public IRenderItem {
	/// The geometry to be drawn.
	const Geometry* geometry = nullptr;

	/// The material that is going to be used when rendering the geometry.
	IMaterialData* pMtlData = nullptr;

	/// The world transformation of the geometry.
	mat4f worldTransform;

	/// The volume that the geometry occupies in world space.
	/// Initially used to obtain witch lights affect the geometry when rendering.
	AABox3f bboxWs;
};

} // namespace sge
