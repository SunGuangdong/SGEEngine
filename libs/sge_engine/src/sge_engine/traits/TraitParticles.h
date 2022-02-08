#pragma once

#include "sge_core/materials/DefaultPBRMtl/DefaultPBRMtl.h"
#include "sge_engine/Actor.h"
#include "sge_engine/AssetProperty.h"
#include "sge_utils/containers/Optional.h"
#include "sge_utils/containers/Pair.h"
#include "sge_utils/math/MultiCurve2D.h"
#include "sge_utils/math/Random.h"
#include "sge_utils/math/Rangef.h"
#include "sge_utils/math/SphericalCoordinates.h"

namespace sge {

struct ICamera;
struct TraitParticlesSimpleRenderItem;
struct TraitParticlesProgrammableRenderItem;

//--------------------------------------------------------------
// ParticleGroupDesc
//--------------------------------------------------------------
struct ParticleBirthCount {
	int instantBirthCount = 0;

	int continiousBirthCount = 0;
	float continiousBirthDuration = 0.f;

	int passiveBirthCountPerSecond = 0;

	float maxLife = 10.f;
	float maxLifeVariation = 0.f;

	float sizeScale = 1.f;
	float sizeScaleVariation = 0.f;
};


//------------------------------------------------------------
// Particle birth shapes.
//------------------------------------------------------------
struct SphereBirthShape {
	// Sphere birth shape settings.
	bool sphereIsVolume = false;
	vec3f sphereCenter = vec3f(0.f);
	float sphereRadius = 1.f;
};

struct PlaneBirthShape {
	// Plane birth shape settings.
	vec3f planePosition = vec3f(0.f);
	SphericalRotation planeRotation = SphericalRotation::axis_neg_y();
	vec2f planeSize = vec2f(1.f);
};

struct PointBirthShape {
	// Point shape settings.
	vec3f pointLocation = vec3f(0.f);
};

struct LineBirthShape {
	// Line shape settings.
	vec3f lineFrom = vec3f(0.f, 0.f, 0.f);
	vec3f lineTo = vec3f(0.f, 1.f, 0.f);
};

struct CircleBirthShape {
	// Circle shape setting.
	bool circleAsDisk = false;
	vec3f circlePosition = vec3f(0.f);
	float circleRadius = 1.f;
	SphericalRotation circleRotation = SphericalRotation::axis_neg_y();
};

enum BirthShape {
	birthShape_sphere,
	birthShape_plane,
	birthShape_point,
	birthShape_line,
	birthShape_circle,
};

struct BirthShapeDesc {
	BirthShape shapeType = birthShape_sphere;

	SphereBirthShape sphere;
	PlaneBirthShape plane;
	PointBirthShape point;
	LineBirthShape line;
	CircleBirthShape circle;
};

//------------------------------------------------------------
// Velocity forces sources.
//------------------------------------------------------------
struct VelocityDirectionalVelocty {
	SphericalRotation directon = SphericalRotation::axis_neg_y();
	float velocityAmount = 1.f;

	// working variables, updated internally.
};

struct VelocityTowardsPoint {
	vec3f pointLocation = vec3f(0.f);
	float velocityAmount = 1.f;
};


struct VelocitySpherical {
	vec3f sphereCenter = vec3f(0.f);
	float velocityAmount = 1.f;
};

enum VeloctyForce : int {
	VelictyForce_none,
	VelictyForce_directional,
	VelictyForce_towardsPoint,
	VelictyForce_spherical,
};

struct Velocity {
	VeloctyForce forceType = VelictyForce_none;

	VelocityDirectionalVelocty directional;
	VelocityTowardsPoint towardsPoint;
	VelocitySpherical spherical;
};

struct ParticlesAlpha {
	float fadeInTimeAfterBirth = 0.f;
	float fadeOutTimeBeforeDeath = 0.f;
};

struct ParticleGroupDesc {
	ParticleBirthCount birth;
	BirthShapeDesc birthShape;
	Velocity initialVelocty;
	std::vector<Velocity> velocityForces;
	vec3f velocityDrag = vec3f(0.f);
	ParticlesAlpha alpha;

	std::string m_name;
	float simulationSpeed = 1.f;

	AssetProperty m_particlesSprite = AssetProperty(assetIface_texture2d);
	vec2i m_spriteGrid = vec2i(1); // The number of sub images in the specified texture by x and y. They go row-wise left to right.
	float m_spritePixelsPerUnit = 32.f;
	float m_spriteFPS = 30.f;
};

//--------------------------------------------------------------
// ParticleGroupState
//--------------------------------------------------------------
struct SGE_ENGINE_API ParticleGroupState {
	struct ParticleState {
		vec3f pos = vec3f(0.f);
		vec3f velocity = vec3f(0.f);
		float scale = 1.f;
		float m_maxLife = 0.f;
		float opacity = 1.f;

		float m_timeSpendAlive = 0.f;
		float m_fSpriteIndex = 0.f; // The sprites are used for rendering this points to the sub-image being used for visualization.

		bool isDead() const
		{
			return m_timeSpendAlive >= m_maxLife;
		}
	};

	// Sprite visulaization mode
	struct SpriteRendData {
		GpuHandle<Buffer> vertexBuffer;
		// GpuHandle<Buffer> indexBuffer;

		Geometry geometry;
		DefaultPBRMtlData material;
	};

	mat4f getParticlesToWorldMtx() const
	{
		return m_isInWorldSpace ? mat4f::getIdentity() : m_n2w;
	}

  private:
	Box3f m_bboxFromLastUpdate;
	Random m_rnd;


	float m_passiveParticleBirthAccumulator = 0.f;
	float m_contiiousParticleBirthAccumulator = 0.f;

	float m_timeRunning = 0.f; // Time in seconds the simulation went running.

	std::vector<ParticleState> m_particles;
	Optional<PerlinNoise3D> m_noise;

	std::vector<Pair<vec2f, vec2f>> spriteFramesUVCache;

	Optional<SpriteRendData> spriteRenderData;
	struct SortingData {
		int index;
		float distanceAlongRay = FLT_MAX;
	};
	std::vector<SortingData> indicesForSorting;

	mat4f m_n2w = mat4f::getIdentity();
	bool m_isInWorldSpace = true;

  public:
	void update(bool isInWorldSpace, const mat4f node2world, const ParticleGroupDesc& spawnDesc, float dt);

	/// @param camera the camera to be used to billboard the particles.
	SpriteRendData* computeSpriteRenderData(SGEContext& sgecon, const ParticleGroupDesc& pdesc, const ICamera& camera);

	const std::vector<ParticleState>& getParticles() const
	{
		return m_particles;
	}

	/// Returns the bounding box in the space they are being simulated (world or node).
	Box3f getBBox() const
	{
		return m_bboxFromLastUpdate;
	}
};

//--------------------------------------------------------------
// TraitParticlesSimple
//--------------------------------------------------------------
ReflAddTypeIdExists(TraitParticlesSimple);
struct SGE_ENGINE_API TraitParticlesSimple : public Trait {
	SGE_TraitDecl_Full(TraitParticlesSimple);

	/// Updates the particles state.
	void update(const GameUpdateSets& u);

	/// Clear the particles simulation state.
	void clearParticleState()
	{
		m_pgroupState.clear();
	}

	Box3f getBBoxOS() const;

	void getRenderItems(std::vector<TraitParticlesSimpleRenderItem>& renderItems);

  public:
	bool m_isEnabled = true;
	bool m_isInWorldSpace = false; // if true, the particles are in world space, false is node space of the owning actor.
	std::vector<ParticleGroupDesc> m_pgroups;
	std::unordered_map<std::string, ParticleGroupState> m_pgroupState;

	// For user interface. Used as the currently edited particle group.
	int m_uiSelectedGroup = 0;
	bool m_isPreviewPlaying = false;
};

//--------------------------------------------------------------
// TraitParticlesProgrammable
//--------------------------------------------------------------
ReflAddTypeIdExists(TraitParticlesProgrammable);
struct SGE_ENGINE_API TraitParticlesProgrammable : public Trait {
	SGE_TraitDecl_BaseFamily(TraitParticlesProgrammable);

	struct ParticleGroup {
		struct ParticleData {
			vec3f position = vec3f(0.f);
			vec3f velocity = vec3f(0.f);
			vec4f tint = vec4f(1.f);
			quatf spin = quatf::getIdentity();
			vec3f angularSpeedAxis = vec3f(0.f);
			float scale = 1.f;
			float angularSpeed = 0.f;
			float age = 0.f;
			float alpha = 1.f; // Controlas the opacity of the particle - 0 transparent, 1 opaque.
			int frameIndex = 0;
			int isDead = false;
			int tag = 0;
		};

		bool isInWorldSpace = true;
		std::vector<ParticleData> allParticles;
		AssetPtr spriteTexture;
		vec2i spriteFramsCount = vec2i(1); /// The number of sub-images in X and Y direction.
		Box3f bbox;                      /// The bounding box of the particles in world or in object space depending on @isInWorldSpace.
		bool needsZAlphaSorting = false;
	};

	void getRenderItems(std::vector<TraitParticlesProgrammableRenderItem>& renderItems);

	virtual int getNumPGroups() const
	{
		return 0;
	}
	virtual ParticleGroup* getPGroup(const int UNUSED(idx))
	{
		return nullptr;
	}
	virtual void update(const GameUpdateSets& UNUSED(u))
	{
	}
};

struct SGE_ENGINE_API ParticleRenderDataGen {
	struct SortingData {
		int index;
		float distanceAlongRay = FLT_MAX;
	};

	struct ParticleVertexData {
		vec3f a_position;
		vec3f a_normal;
		vec4f a_color;
		vec2f a_uv;
	};

  public:
	/// Returns true if there was data ready for rendering and it succeeded.
	bool generate(const TraitParticlesProgrammable::ParticleGroup& particles, SGEContext& sgecon, const ICamera& camera, const mat4f& n2w);

	std::vector<SortingData> indicesForSorting;
	std::vector<Pair<vec2f, vec2f>> spriteFramesUVCache;
	std::vector<ParticleVertexData> vertexBufferData;

	GpuHandle<Buffer> vertexBuffer;
	Geometry geometry;
	DefaultPBRMtlData material;
};

} // namespace sge
