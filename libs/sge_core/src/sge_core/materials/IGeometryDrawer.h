#pragma once

#include "sge_core/sgecore_api.h"
#include "sge_utils/math/mat4.h"
#include "sge_utils/math/Box.h"

namespace sge {

struct Texture;
struct Geometry;
struct LightDesc;
struct IMaterialData;

//------------------------------------------------------------
// ShadingLightData
// Describes the light data in a converted form, suitable for
// passing it toe shader
//------------------------------------------------------------
struct ShadingLightData {
	const LightDesc* pLightDesc = nullptr;
	Texture* shadowMap = nullptr;
	AABox3f lightBoxWs;
	mat4f shadowMapProjView = mat4f::getIdentity();
	vec3f lightPositionWs = vec3f(0.f);
	vec3f lightDirectionWs = vec3f(0.f);
};

struct InstanceDrawMods {
	mat4f uvwTransform = mat4f::getIdentity();
	float gameTime = 0.f;
	bool forceNoLighting = false;
	bool forceAdditiveBlending = false;
	bool forceNoCulling = false;
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

struct ICamera;
struct RenderDestination;
struct EvaluatedModel;

struct SGE_CORE_API IGeometryDrawer {
	IGeometryDrawer() = default;
	virtual ~IGeometryDrawer() = default;

	virtual void drawGeometry(const RenderDestination& rdest,
	                          const ICamera& camera,
	                          const mat4f& geomWorldTransfrom,
	                          const ObjectLighting& lighting,
	                          const Geometry& geometry,
	                          const IMaterialData* mtlDataBase,
	                          const InstanceDrawMods& instDrawMods) = 0;
};


SGE_CORE_API void drawGeometry(const RenderDestination& rdest,
                               const ICamera& camera,
                               const mat4f& geomWorldTransfrom,
                               const ObjectLighting& lighting,
                               const Geometry& geometry,
                               const IMaterialData* mtlDataBase,
                               const InstanceDrawMods& instDrawMods);

SGE_CORE_API void drawEvalModel(const RenderDestination& rdest,
                                const ICamera& camera,
                                const mat4f& geomWorldTransfrom,
                                const ObjectLighting& lighting,
                                const EvaluatedModel& evalModel,
                                const InstanceDrawMods& instDrawMods);


} // namespace sge
