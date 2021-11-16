#pragma once

#include "ShadingProgramPermuator.h"
#include "sge_core/materials/IGeometryDrawer.h"
#include "sge_core/model/EvaluatedModel.h"
#include "sge_core/sgecore_api.h"
#include "sge_core/shaders/LightDesc.h"
#include "sge_utils/math/mat4.h"
#include "sge_utils/utils/OptionPermutator.h"
#include "sge_utils/utils/optional.h"

namespace sge {

struct EvaluatedModel;
struct ModelMesh;
//------------------------------------------------------------
// BasicModelDraw
//------------------------------------------------------------
struct SGE_CORE_API BasicModelDraw : public IGeometryDrawer {
  public:
	BasicModelDraw() = default;

	virtual void drawGeometry(const RenderDestination& rdest,
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
