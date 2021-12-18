#pragma once

#include "sge_core/sgecore_api.h"
#include "sge_utils/math/mat4.h"
#include "sge_utils/utils/span.h"
#include <array>
#include <string>
#include <unordered_map>
#include <vector>


namespace sge {

struct EvaluatedModel;
struct Model;

/// @brief Describes what should happen when an animation track finishes.
enum TrackTransition : int {
	/// The animation should continue to loop when it ends. Example idle animation.
	trackTransition_loop,

	/// The animation should stop at the end frame. Example death animation.
	trackTransition_stop,

	/// The animation should switch to another track when it ends.
	/// Example is when jump animation ends usually
	/// we want to be in landing anticipation animation.
	trackTransition_switchTo,
};

struct SGE_CORE_API ModelAnimator2 {
	/// Initializes the @ModelAnimator2 so it could animate the nodes to the specified model.
	void create(Model& modelToBeAnimated);

	/// Create a new track.
	/// @param [in] trackId is a user specified number, identifying the track.
	/// A track is a set of animations that should are logically the same.
	/// For example a character might have multiple idle animations to add variations.
	/// The track just combines all of them in one set.
	void trackCreate(int trackId);

	/// Add an animation source to the specified track, so when playing this track
	/// the animation would have the option to choose this animation.
	/// @trackCreate should already be called for the speicifed @trackId.
	/// @param [in] srcModel is the model that is going to provide the animation with name
	/// @animatioNameInSrcModel. If you want to use the model passed to @create just pass nullptr.
	void trackAddAmim(int trackId, Model* srcModel, const char* animationNameInSrcModel);

	void trackAddAmim(int trackId, Model* srcModel, int animIndexInSrcModel);

	/// Sets the current playing track to the specified one.
	void playTrack(int trackIdToPlay);

	void forceTrack(int trackIdToPlay, float animTime);

	void advanceAnimation(const float dt);

	/// @param [out] outNodeTransforms a pre-allocated array holding a trasnform for each node.
	/// The size could be obtained by @getNumNodes.
	void computeModleNodesTrasnforms(mat4f* outNodeTransforms);

	/// @param [out] outNodeTransforms will be automatically resized to hold a transform for each node.
	void computeModleNodesTrasnforms(std::vector<mat4f>& outNodeTransforms) {
		outNodeTransforms.resize(getNumNodes());
		computeModleNodesTrasnforms(outNodeTransforms.data());
	}

	int getNumNodes() const;

	int getNumTacks() const {
		return int(m_tracks.size());
	}

	int getPlayingTrackId() const {
		if (!m_playbacks.empty())
			return m_playbacks.back().trackId;

		return -1;
	}

  private:
	/// Create a @srcModel to @m_modelToAnimate node id remapping.
	/// Used to find the matching nodes.
	void createNodeToNodeRemapForModel(Model& srcModel);

  private:
	/// While models have animations in them it is not uncommon,
	/// to share animations between multiple models.
	/// It is common to have multiple character files that have
	/// the same rig (node hierarchy used for bones) and have a separate animation file
	/// that we can apply these 3D models.
	/// This is the reason of this structure, it describe the model which we wanna use
	/// to 'steal' animation from.
	struct AnimationModelSrc {
		Model* modelAnimSouce = nullptr;
		int animIndexInAnimSource = -1;
	};

	/// Describes a single track and the animations that are associated with it.
	struct AnimationTrack {
		std::vector<AnimationModelSrc> animationSources;

		/// Determines what should happen when this track finishes.
		TrackTransition transition = trackTransition_loop;

		/// If the @transition is set to animationTransition_switchTo
		/// then this hold the id of the animation that we need to switch to.
		/// Unused otherwise.
		int switchToTrackId = -1;

		/// The time needed to blend from and to this track form other animation track.
		float fadeInOutTime = 0.f;
	};

	/// Describes the state of a track being currently played.
	struct TrackPlayback {
		int trackId = -1;
		int trackAnimationIndex = -1; // The index in @AnimationTrack::animationSources that provides the animation.
		float timeInAnimation = 0.f;

		/// Unormalzed weight, used when mixing multiple track playbacks.
		/// This is for internal use.
		/// Use @normWeight to use is as a weight when combining multiple tracks(animations).
		float mixingWeight = 1.f;

		/// The weight of the track to be used when computing the nodes trasnform.
		/// It is computed in such a way that the sum of all of these for all playing tracks is 1.f.
		float normWeight = 0.f;
	};

  private:
	Model* m_modelToAnimate = nullptr;

	/// The descritions for all available tracks.
	std::unordered_map<int, AnimationTrack> m_tracks;

	float fadeoutTimeTotal = 0.f;

	/// The state of each playing track.
	/// There could be multiple tracks fading in/out while switching them.
	/// This is a stack so the last one is "dominant".
	std::vector<TrackPlayback> m_playbacks;

	std::unordered_map<const Model*, std::vector<int>> m_perModel_srcNode_toNode;
};

} // namespace sge
