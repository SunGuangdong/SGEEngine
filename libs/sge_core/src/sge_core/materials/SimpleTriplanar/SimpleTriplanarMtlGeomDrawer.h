#pragma once

#include "sge_core/materials/IGeometryDrawer.h"
#include "sge_core/sgecore_api.h"
#include "sge_core/shaders/LightDesc.h"
#include "sge_core/shaders/ShadingProgramPermuator.h"
#include "sge_renderer/renderer/renderer.h"
#include "sge_utils/containers/Optional.h"
#include "sge_utils/math/mat4f.h"
#include "sge_utils/other/OptionPermutator.h"

namespace sge {

//------------------------------------------------------------
// DefaultPBRMtlGeomDrawer
//------------------------------------------------------------
struct SGE_CORE_API SimpleTriplanarMtlGeomDrawer : public IGeometryDrawer {
	SimpleTriplanarMtlGeomDrawer() = default;

	virtual void drawGeometry(
	    const RenderDestination& rdest,
	    const ICamera& camera,
	    const mat4f& geomWorldTransfrom,
	    const ObjectLighting& lighting,
	    const Geometry& geometry,
	    const IMaterialData* mtlDataBase,
	    const InstanceDrawMods& instDrawMods) override;


  private:
	Optional<ShadingProgramPermuator> shadingPermutFWDShading;
	GpuHandle<Buffer> paramsBuffer;
	StateGroup stateGroup;
};

} // namespace sge
