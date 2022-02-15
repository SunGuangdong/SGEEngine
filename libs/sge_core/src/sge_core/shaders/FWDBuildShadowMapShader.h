#pragma once

#include "ShadingProgramPermuator.h"
#include "sge_core/model/EvaluatedModel.h"
#include "sge_core/sgecore_api.h"
#include "sge_utils/containers/Optional.h"
#include "sge_utils/math/mat4f.h"
#include "sge_utils/other/OptionPermutator.h"

namespace sge {

struct ShadowMapBuildInfo;

struct SGE_CORE_API FWDBuildShadowMapShader {
	FWDBuildShadowMapShader() = default;

	void drawGeometry(
	    const RenderDestination& rdest,
	    const vec3f& camPos,
	    const mat4f& projView,
	    const mat4f& world,
	    const ShadowMapBuildInfo& shadowMapBuildInfo,
	    const Geometry& geometry,
	    const Texture* diffuseTexForAlphaMask,
	    const bool forceNoCulling);

  private:
	Optional<ShadingProgramPermuator> shadingPermutFWDBuildShadowMaps;
	StateGroup stateGroup;
};

} // namespace sge
