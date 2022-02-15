#pragma once

#include "DetourNavMesh.h"
#include "RecastDetourWrapper.h"
#include "sge_engine/Actor.h"
#include "sge_engine/traits/TraitCustomAE.h"
#include "sge_engine/traits/TraitViewportIcon.h"
#include "sge_utils/react/Event.h"

namespace sge {

struct NavMeshBuildSets {
	float cellXZSize = 0.25f;
	float cellYSize = 0.1f;
	float climbableSlopeAngle = deg2rad(45.f);
	float cimbableStairHeight = 0.2f;
	// Imagine that the mesh is a multiple stories flat, recast needs to know how high the rooms are.
	float minRoomHeight = 0.1f;

	// How wide an area should be to be concidered walkable.
	float agentRadius = 0.01f;
	float agentHeight = 0.005f;

	float polygonsMaxEdgeLength = 10.f;
};

struct SGE_ENGINE_API INavMesh {
	INavMesh() = default;
	virtual ~INavMesh() = default;

	virtual bool findPath(
	    std::vector<vec3f>& outPath,
	    const vec3f& startPos,
	    const vec3f& endPos,
	    const vec3f nearestPointSearchHalfDiagonal = vec3f(0.5f)) = 0;

	virtual vec3f moveAlongNavMesh(const vec3f& start, const vec3f& end) = 0;
};

//--------------------------------------------------------
// ANavMesh
//--------------------------------------------------------
struct SGE_ENGINE_API ANavMesh : public Actor, public IActorCustomAttributeEditorTrait, public INavMesh {
	ANavMesh() = default;

	void create() final;
	Box3f getBBoxOS() const final;
	void update(const GameUpdateSets& updateSets) final;

	void build();

	// From IActorCustomAttributeEditorTrait:
	void doAttributeEditor(GameInspector* inspector) override;

	// From INavMesh:
	bool findPath(
	    std::vector<vec3f>& outPath,
	    const vec3f& startPos,
	    const vec3f& endPos,
	    const vec3f nearestPointSearchHalfDiagonal = vec3f(0.5f)) final;


	vec3f moveAlongNavMesh(const vec3f& start, const vec3f& end) final;

  private:
	void clearRecastAndDetourState()
	{
		m_detourNavMesh.freeExisting();
		m_detourNavMeshQuery.freeExisting();

		m_debugDrawNavMeshBuildTriListWs = std::vector<vec3f>();
		m_debugDrawNavMeshTriListWs = std::vector<vec3f>();
	}

  public:
	NavMeshBuildSets m_buildSettings;
	rcPolyMeshWrapper m_recastPolyMesh;

	dtNavMeshWrapper m_detourNavMesh;
	dtNavMeshQueryWrapper m_detourNavMeshQuery;

	TraitViewportIcon m_traitViewportIcon;

	std::vector<vec3f> m_debugDrawNavMeshBuildTriListWs;
	std::vector<vec3f> m_debugDrawNavMeshTriListWs;

	std::vector<TypeId> m_typesToUse;

	EventSubscription onWorldLoadedCBHandle;
};

} // namespace sge
