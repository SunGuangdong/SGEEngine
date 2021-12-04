#pragma once

#include "sge_core/sgecore_api.h"
#include "sge_utils/math/Box.h"
#include "sge_utils/math/mat4.h"

namespace sge {

struct Texture;
struct Geometry;
struct LightDesc;
struct IMaterialData;

/// This structure is currently empty, but
/// pre-material-as-assets it was used to override some settings
/// like disabling culling/lighting and so on.
/// Currently it is not used for anything and hopefully it will get deleted.
/// However, I feel that I'll need to reintroduce it once more this is why I've kept it.
struct InstanceDrawMods {};

/// ShadingLightData describes a single light settings
/// to be used by the @IGeometryDrawer when rendering a single geometry.
struct ShadingLightData {
	const LightDesc* pLightDesc = nullptr;
	Texture* shadowMap = nullptr;
	AABox3f lightBoxWs;
	mat4f shadowMapProjView = mat4f::getIdentity();
	vec3f lightPositionWs = vec3f(0.f);
	vec3f lightDirectionWs = vec3f(0.f);
};

/// ObjectLighting describes all the lights that should be used
/// when rendering a specific geomety. It is up to the @IGeometryDrawer that
/// uses them to intepret the data whatever how it wants.
struct ObjectLighting {
	/// The ambient light color and intensity to be applied to the object.
	/// Usually this is the same as the one in the scene.
	vec3f ambientLightColor = vec3f(1.f);

	/// How much fake abient detail should be added (by using the normals).
	/// 0 means no abient detail and unlit surfaces would look flat.
	/// 1 means 100% fake detail, altering the ambient lighting
	/// where then normals chage.
	float ambientFakeDetailBias = 0.f;

	/// The rim light in the scene.
	vec4f uRimLightColorWWidth = vec4f(vec3f(0.1f), 0.7f);

	/// lightsCount is the size of the array pointed by @ppLightData.
	int lightsCount = 0;

	/// An array of all lights that affect the object. The size of the array is @lightsCount.
	const ShadingLightData** ppLightData = nullptr;

	static ObjectLighting getAmbientLightOnly() {
		ObjectLighting result;
		// 2.f is used instead of one to compensate for the dimming of ambientFakeDetailBias.
		result.ambientLightColor = vec3f(2.f);
		result.ambientFakeDetailBias = 1.f;

		return result;
	}
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
