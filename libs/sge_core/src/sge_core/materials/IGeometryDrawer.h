#pragma once

#include "sge_core/sgecore_api.h"
#include "sge_utils/math/Box3f.h"
#include "sge_utils/math/mat4f.h"

namespace sge {

struct Texture;
struct Geometry;
struct LightDesc;
struct IMaterialData;

/// This structure is currently empty, but
/// pre-material-as-assets changes it was used to override some settings
/// like disabling culling/lighting and so on.
/// Currently it is not used for anything and hopefully it will get deleted.
/// However, I feel that I'll need to reintroduce it once more this is why I've kept it.
struct InstanceDrawMods {
};

/// ShadingLightData describes a single light settings
/// to be used by the @IGeometryDrawer when rendering a single geometry.
struct ShadingLightData {
	const LightDesc* pLightDesc = nullptr;
	Texture* shadowMap = nullptr;
	Box3f lightBoxWs;
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
	/// depending on the direction of the shading normal in world space.
	float ambientFakeDetailBias = 0.f;

	/// lightsCount is the size of the array pointed by @ppLightData.
	int lightsCount = 0;

	/// An array of all lights that affect the object.
	/// The size of the array is @lightsCount.
	const ShadingLightData** ppLightData = nullptr;

	static ObjectLighting makeAmbientLightOnly()
	{
		ObjectLighting result;
		result.ambientLightColor = vec3f(1.f);
		result.ambientFakeDetailBias = 1.f;

		return result;
	}
};

struct ICamera;
struct RenderDestination;
struct EvaluatedModel;

/// IGeometryDrawer is an interface that is implemented for each material type.
/// It is a part of the abstraction for rendering different materials via one interface.
/// It provides a way to render the specified geometry with the material that this class
/// is implemented for.
/// By doing this the game code could just call the global @drawGeometry and it will
/// automatically find the @IGeometryDrawer for the specified material.
/// You can still call this manually if you want.
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

/// Draw the specified geometry(mesh) with the specified material.
/// The function will find the correct @IGeometryDrawer automatically for the material.
SGE_CORE_API void drawGeometry(const RenderDestination& rdest,
                               const ICamera& camera,
                               const mat4f& geomWorldTransfrom,
                               const ObjectLighting& lighting,
                               const Geometry& geometry,
                               const IMaterialData* mtlDataBase,
                               const InstanceDrawMods& instDrawMods);

/// A short function for drawing a evaluated model as it is.
/// The function will find the correct @IGeometryDrawer automatically for each material.
SGE_CORE_API void drawEvalModel(const RenderDestination& rdest,
                                const ICamera& camera,
                                const mat4f& geomWorldTransfrom,
                                const ObjectLighting& lighting,
                                const EvaluatedModel& evalModel,
                                const InstanceDrawMods& instDrawMods);


} // namespace sge
