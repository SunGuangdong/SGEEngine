#pragma once

#include "sge_engine/Actor.h"
#include "sge_engine/traits/TraitCustomAE.h"
#include "sge_engine/traits/TraitViewportIcon.h"

namespace sge {

//--------------------------------------------------------------------
// AIKNode
//--------------------------------------------------------------------
struct SGE_ENGINE_API AIKNode : public Actor, public IActorCustomAttributeEditorTrait {
	// Create the ridig body for the actor. The origin is picked to be the bottom mid point
	// so it could be easier to place it over other objects.
	static inline const Box3f kBBoxObjSpace = Box3f::getFromHalfDiagonal(vec3f(0.5f), vec3f(0.5f));

	void create() final;
	Box3f getBBoxOS() const final;

	void update(const GameUpdateSets& u) override;

	virtual void doAttributeEditor(GameInspector* inspector) final;

  private:
	void bind(ObjectId start, ObjectId end);
	void unbind()
	{
		ikChainActorIds.clear();
		linksLengthWs.clear();
		chainLengthWs = 0.f;
		targetMatchOrientationDiff = quatf::getIdentity();
	}

	// Returns true if all actors actually exist.
	bool getChainActors(std::vector<Actor*>& result);

  public:
	TraitViewportIcon m_traitViewportIcon;

	bool isEnabled = false;
	int maxIterations = 10;
	float earlyExitDelta = 1e-3f;
	bool targetControlsOrientation = false;
	ObjectId startId;
	ObjectId endId;
	ObjectId targetId;
	ObjectId poleId;
	bool useNonInstantFollow = false;
	float nonInstantFollowSpeed = 1.f;

	// Bindings.
	std::vector<ObjectId> ikChainActorIds;
	std::vector<float> linksLengthWs;
	std::vector<transf3d> initalOrientationsWs;
	float chainLengthWs = 0.f;
	quatf targetMatchOrientationDiff = quatf::getIdentity();

	// Solver comutation variables stoed here to avoid unnecessary allocations.
	std::vector<Actor*> tempChainActors;
	std::vector<vec3f> solverPositionWs;
};

} // namespace sge
