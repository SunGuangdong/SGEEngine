#pragma once

#include "sge_core/materials/IGeometryDrawer.h"
#include "sge_core/sgecore_api.h"
#include "sge_core/shaders/LightDesc.h"
#include "sge_core/shaders/ShadingProgramPermuator.h"
#include "sge_renderer/renderer/renderer.h"
#include "sge_utils/containers/Optional.h"
#include "sge_utils/io/FileWatcher.h"
#include "sge_utils/math/mat4f.h"
#include "sge_utils/other/OptionPermutator.h"

namespace sge {

//------------------------------------------------------------
// DefaultPBRMtlGeomDrawer
//------------------------------------------------------------
struct SGE_CORE_API DefaultPBRMtlGeomDrawer : public IGeometryDrawer {
	DefaultPBRMtlGeomDrawer() = default;

	virtual void drawGeometry(
	    const RenderDestination& rdest,
	    const ICamera& camera,
	    const mat4f& geomWorldTransfrom,
	    const ObjectLighting& lighting,
	    const Geometry& geometry,
	    const IMaterialData* mtlDataBase,
	    const InstanceDrawMods& instDrawMods) override;


  private:
	std::unordered_map<std::string, Optional<ShadingProgramPermuator>> shadingPermutFWDShadingFilename;
	GpuHandle<Buffer> paramsBuffer;
	StateGroup stateGroup;

	FilesWatcher shaderFilesWatcher;
};

} // namespace sge
