#include "TraitRenderGeometry.h"
#include "sge_core/materials/IMaterial.h"

namespace sge {
ReflAddTypeId(TraitRenderGeometry, 20'12'09'0001);


void TraitRenderGeometry::getRenderItems(DrawReason UNUSED(drawReason), std::vector<GeometryRenderItem>& renderItems)
{
	if (geoms.empty()) {
		return;
	}

	mat4f actor2world = getActor()->getTransformMtx();

	for (Element& geom : geoms) {
		if (geom.isRenderable) {
			GeometryRenderItem ri;
			ri.geometry = geom.pGeom;
			ri.pMtlData = geom.pMtl;
			if (geom.isTformInWorldSpace) {
				ri.worldTransform = geom.tform;
			}
			else {
				ri.worldTransform = actor2world * geom.tform;
			}
			ri.bboxWs = geom.bboxGeometry.getTransformed(ri.worldTransform);

			ri.zSortingPositionWs = ri.bboxWs.center();
			ri.needsAlphaSorting = ri.pMtlData->needsAlphaSorting;

			renderItems.push_back(ri);
		}
	}
}

} // namespace sge
