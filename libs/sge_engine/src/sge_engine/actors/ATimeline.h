#pragma once

#include "sge_engine/Actor.h"
#include "sge_engine/GameInspector.h"
#include "sge_engine/traits/TraitCustomAE.h"
#include "sge_engine/traits/TraitModel.h"
#include "sge_engine/traits/TraitPath.h"
#include "sge_engine/traits/TraitViewportIcon.h"
#include "sge_utils/utils/vector_map.h"

namespace sge {

//--------------------------------------------------------
// ATimeline
//--------------------------------------------------------
struct ATimeline : public Actor, public IActorCustomAttributeEditorTrait {
	enum PlaybackMethod : int {
		playbackMethod_reset,
		playbackMethod_stop,
		playbackMethod_flipflop,
	};

	AABox3f getBBoxOS() const override;

	void create() override;
	void postUpdate(const GameUpdateSets& updateSets) override;

	// IActorCustomAttributeEditorTrait
	void doAttributeEditor(GameInspector* inspector) override;

	float getRawAnimationLength() const {
		const float res = keyFrames.size() == 0 ? 0.f : keyFrames.getAllKeys().back();
		return res;
	}

  public:
	TraitViewportIcon ttViewportIcon;

	bool relativeMode = false;   ///< If true, the animation should be played relative to the timeline node.
	transf3d relativeModeOrigin; ///< This is the transform that we are going to use to make the keyfames

	bool isInEditMode = false;
	bool doesEditModeNeedsUpdate = false;

	bool m_isEnabled = false;
	bool m_useSmoothInterpolation = false;
	float m_gameplayEvalTime = 0.f;
	float m_editingEvaltime = 0;
	bool moveObjectsOnTop = false;

	PlaybackMethod playbackMethod = playbackMethod_reset;
	float flipFlopDir = 1.f;

	int framesPerSecond = 30;
	int frameCount = 30;

	vector_set<ObjectId> affectedActorsIds;

	vector_map<int, vector_map<ObjectId, transf3d>> keyFrames;
};


} // namespace sge
