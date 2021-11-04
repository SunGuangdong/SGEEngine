#pragma once

#include "ShadingProgramPermuator.h"
#include "sge_core/model/EvaluatedModel.h"
#include "sge_core/sgecore_api.h"
#include "sge_utils/math/mat4.h"
#include "sge_utils/utils/OptionPermutator.h"
#include "sge_utils/utils/optional.h"

namespace sge {

struct EvaluatedModel;
struct ModelMesh;

struct MaterialOverride {
	std::string name;
	PBRMaterial mtl;
};

//------------------------------------------------------------
// ShadingLightData
// Describes the light data in a converted form, suitable for
// passing it toe shader
//------------------------------------------------------------
struct ShadingLightData {
	/// (x,y,z) - position or direction in case of directional light.
	/// w holds type of the light (a member of LightType) as a float.
	vec4f lightPositionAndType = vec4f(0.f);
	vec4f lightSpotDirAndCosAngle = vec4f(1.f, 0.f, 0.f, 0.f);
	// (x,y,z) the color of the light multiplied by the intensity). w - flags as float.
	vec4f lightColorWFlags = vec4f(0.f);
	float shadowMapBias = 0.f;

	Texture* shadowMap = nullptr;
	mat4f shadowMapProjView = mat4f::getIdentity();
	// x is the light shadow range, (y,z,w) unused.
	vec4f lightXShadowRange = vec4f(0.f);

	AABox3f lightBoxWs;
};

//------------------------------------------------------------
// ObjectLighting
//------------------------------------------------------------
struct ObjectLighting {
	/// The ambient light color and intensity to be applied to the object.
	/// Usually this is the same as the one in the scene.
	vec3f ambientLightColor = vec3f(1.f);

	/// The rim light in the scene.
	vec4f uRimLightColorWWidth = vec4f(vec3f(0.1f), 0.7f);

	/// lightsCount is the size of the array pointed by @ppLightData.
	int lightsCount = 0;

	/// An array of all lights that affect the object. The size of the array is @lightsCount.
	const ShadingLightData** ppLightData = nullptr;
};

struct InstanceDrawMods {
	mat4f uvwTransform = mat4f::getIdentity();
	float gameTime = 0.f;
	bool forceNoLighting = false;
	bool forceAdditiveBlending = false;
	bool forceNoCulling = false;
};

//------------------------------------------------------------
// BasicModelDraw
//------------------------------------------------------------
struct SGE_CORE_API BasicModelDraw {
  public:
	BasicModelDraw() = default;

	void draw(const RenderDestination& rdest,
	          const vec3f& camPos,
	          const vec3f& camLookDir,
	          const mat4f& projView,
	          const mat4f& preRoot,
	          const ObjectLighting& lighting,
	          const EvaluatedModel& model,
	          const InstanceDrawMods& mods,
	          const std::vector<MaterialOverride>* mtlOverrides = nullptr);

	void drawGeometry(const RenderDestination& rdest,
	                  const vec3f& camPos,
	                  const vec3f& camLookDir,
	                  const mat4f& projView,
	                  const mat4f& world,
	                  const ObjectLighting& lighting,
	                  const Geometry* geometry,
	                  const PBRMaterial& material,
	                  const InstanceDrawMods& mods);

  private:
	void drawGeometry_FWDShading(const RenderDestination& rdest,
	                             const vec3f& camPos,
	                             const vec3f& camLookDir,
	                             const mat4f& projView,
	                             const mat4f& world,
	                             const ObjectLighting& lighting,
	                             const Geometry* geometry,
	                             const PBRMaterial& material,
	                             const InstanceDrawMods& mods);

  private:
	Optional<ShadingProgramPermuator> shadingPermutFWDShading;
	Optional<ShadingProgramPermuator> shadingPermutFWDBuildShadowMaps;
	GpuHandle<Buffer> paramsBuffer;
	StateGroup stateGroup;
};

} // namespace sge
