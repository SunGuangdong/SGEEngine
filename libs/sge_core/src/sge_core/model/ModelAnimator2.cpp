#include "ModelAnimator2.h"
#include "sge_utils/utils/range_loop.h"
#include <functional>

namespace sge {


void ModelAnimator2::create(Model& modelToBeAnimated) {
	*this = ModelAnimator2();
	this->m_modelToAnimate = &modelToBeAnimated;
}

void ModelAnimator2::trackCreate(int trackId) {
	if (m_tracks.count(trackId) == 0) {
		m_tracks[trackId] = AnimationTrack();
	} else {
		sgeAssertFalse("A track with this id has already been created");
	}
}

void ModelAnimator2::trackAddAmim(int trackId, Model* srcModel, const char* animationNameInSrcModel) {
	if (animationNameInSrcModel == nullptr) {
		sgeAssertFalse("Animation name is empty.");
		return;
	}

	int animIndexInAnimSource = -1;
	if (srcModel) {
		animIndexInAnimSource = srcModel->getAnimationIndexByName(animationNameInSrcModel);
	} else {
		animIndexInAnimSource = m_modelToAnimate->getAnimationIndexByName(animationNameInSrcModel);
	}

	if (animIndexInAnimSource < 0) {
		sgeAssertFalse("Animation with the specified name is not found.");
		return;
	}

	// Add the animation to the track.
	trackAddAmim(trackId, srcModel, animIndexInAnimSource);
}

void ModelAnimator2::trackAddAmim(int trackId, Model* srcModel, int animIndexInSrcModel) {
	AnimationTrack& track = m_tracks[trackId];
	track.animationSources.emplace_back(AnimationModelSrc{srcModel, animIndexInSrcModel});

	// Create srcModel to m_modelToAnimate node ids remapping.
	if (srcModel) {
		createNodeToNodeRemapForModel(*srcModel);
	}
}

void ModelAnimator2::createNodeToNodeRemapForModel(Model& srcModel) {
	if (m_perModel_srcNode_toNode.count(&srcModel) > 0) {
		return;
	}

	std::vector<int> srcNode_toNode;
	srcNode_toNode.resize(m_modelToAnimate->numNodes(), -1);


	// Build the node-to-node remapping, Keep in mind that some nodes might not be present in the donor.
	// in that case -1 should be written for that node index.
	for (const int iOrigNode : range_int(m_modelToAnimate->numNodes())) {
		const std::string& originalNodeName = m_modelToAnimate->nodeAt(iOrigNode)->name;
		srcNode_toNode[iOrigNode] = srcModel.findFistNodeIndexWithName(originalNodeName);
	}

	m_perModel_srcNode_toNode[&srcModel] = std::move(srcNode_toNode);
}

void ModelAnimator2::playTrack(int trackIdToPlay) {
	// If we are already playing the same track as the top of the stack, do nothing.
	if (getPlayingTrackId() == trackIdToPlay) {
		return;
	}


	const auto& itrTrackDesc = m_tracks.find(trackIdToPlay);

	if (itrTrackDesc == m_tracks.end()) {
		sgeAssert(false && "Invalid track id.");
		return;
	}

	fadeoutTimeTotal = itrTrackDesc->second.fadeInOutTime;
	if (fadeoutTimeTotal == 0.f) {
		m_playbacks.clear();
		fadeoutTimeTotal = 0.f;
	}

	TrackPlayback playback;

	playback.trackId = trackIdToPlay;
	playback.timeInAnimation = 0.f;
	playback.iAnimation = 0; // TODO: randomize this.
	// If there is going to be a fadeout start form 0 weight growing to one, to smoothly transition.
	// otherwise durectly use 100% weight.
	playback.mixingWeight = m_playbacks.empty() ? 1.f : 0.f;

	m_playbacks.emplace_back(playback);
}

void ModelAnimator2::forceTrack(int trackIdToPlay, float animTime) {
	m_playbacks.clear();

	TrackPlayback playback;
	playback.trackId = trackIdToPlay;
	playback.timeInAnimation = 0.f;
	playback.iAnimation = 0;
	playback.mixingWeight = 1.f;
	playback.normWeight = 1.f;

	m_playbacks.emplace_back(playback);
}

void ModelAnimator2::advanceAnimation(const float dt) {
	if (m_playbacks.empty()) {
		return;
	}

	int switchToPlayTrackId = -1;

	// Update the tracks progress.
	for (int iPlayback = 0; iPlayback < m_playbacks.size(); ++iPlayback) {
		TrackPlayback& playback = m_playbacks[iPlayback];

		const AnimationTrack& trackInfo = m_tracks[playback.trackId];

		const AnimationModelSrc& animationSrc = trackInfo.animationSources[playback.iAnimation];
		const ModelAnimation* const animInfo = animationSrc.modelAnimSouce->animationAt(animationSrc.animIndexInAnimSource);

		if (animInfo == nullptr) {
			// This should never happen, all the animations should be available.
			// Handle this so we do not crash.
			sgeAssert(false);
			m_playbacks.erase(m_playbacks.begin() + iPlayback);
			--iPlayback;
			continue;
		}

		// Advance the playback time.
		playback.timeInAnimation += dt;

		int const signedRepeatCnt = (int)(playback.timeInAnimation / animInfo->durationSec);
		int const repeatCnt = abs(signedRepeatCnt);

		// In that case the animation has ended. Handle the track transitions.
		if (repeatCnt != 0) {
			// True if the track playing is complete and needs to get removed.
			bool trackNeedsRemoving = false;

			switch (trackInfo.transition) {
				case trackTransition_loop: {
					playback.timeInAnimation = playback.timeInAnimation - animInfo->durationSec * (float)signedRepeatCnt;
					playback.iAnimation = 0; // TODO: randomize the animation.
				} break;
				case trackTransition_stop: {
					playback.timeInAnimation = animInfo->durationSec;
				} break;
				case trackTransition_switchTo: {
					// In that case we need to switch to another track if we aren't already in it.
					if (getPlayingTrackId() != playback.trackId && trackInfo.switchToTrackId != playback.trackId &&
					    trackInfo.switchToTrackId != -1) {
						switchToPlayTrackId = trackInfo.switchToTrackId;
					}

					// Finally remove that track it is no longer needed we assume that the new
					// track that we've switched to blends perfectly.
					trackNeedsRemoving = true;

				} break;

				default:
					// Unimplemented transition, assert and make it stop.
					sgeAssert(false && "Unimplemented transiton");
					playback.timeInAnimation = animInfo->durationSec;
					break;
			}

			if (trackNeedsRemoving) {
				m_playbacks.erase(m_playbacks.begin() + iPlayback);
				--iPlayback;
				continue;
			}
		}
	}

	if (switchToPlayTrackId != -1) {
		playTrack(switchToPlayTrackId);
	}

	// Update the track weights.
	if (fadeoutTimeTotal > 1e-6f) {
		const float fadePercentage = dt / fadeoutTimeTotal;

		for (int iPlayback = 0; iPlayback < m_playbacks.size(); ++iPlayback) {
			TrackPlayback& playback = m_playbacks[iPlayback];

			// The last element here is the primary track. It is fading in.
			// All others are fading out.
			if (iPlayback != (int(m_playbacks.size()) - 1)) {
				playback.mixingWeight -= fadePercentage;
				if (playback.mixingWeight <= 0.f) {
					m_playbacks.erase(m_playbacks.begin() + iPlayback);
					--iPlayback;
					continue;
				}
			} else {
				playback.mixingWeight += fadePercentage;
				playback.mixingWeight = clamp01(playback.mixingWeight);
			}
		}
	}

	if (m_playbacks.size() == 1) {
		m_playbacks[0].mixingWeight = 1.f;
		m_playbacks[0].normWeight = 1.f;
	} else {
		float totalWeigth = 0.f;
		for (int iPlayback = 0; iPlayback < m_playbacks.size(); ++iPlayback) {
			TrackPlayback& playback = m_playbacks[iPlayback];
			const AnimationTrack& trackInfo = m_tracks[playback.trackId];

			totalWeigth += playback.mixingWeight;
		}

		for (int iPlayback = 0; iPlayback < m_playbacks.size(); ++iPlayback) {
			TrackPlayback& playback = m_playbacks[iPlayback];
			m_playbacks[iPlayback].normWeight = m_playbacks[iPlayback].mixingWeight / totalWeigth;
		}
	}
}

void ModelAnimator2::computeModleNodesTrasnforms(span<mat4f>& outNodeTransforms) {
	if (outNodeTransforms.size() < m_modelToAnimate->numNodes()) {
		sgeAssertFalse("The outNodeTransforms is not big enough!");
		return;
	}

	for (mat4f& m : outNodeTransforms) {
		m = mat4f::getZero();
	}

	// Evaluates the nodes. They may be effecte by multiple models (stealing animations and blending them)
	for (const TrackPlayback& playback : m_playbacks) {
		const AnimationTrack& track = m_tracks[playback.trackId];
		const AnimationModelSrc& animSrc = track.animationSources[playback.iAnimation];

		const Model& animationSourceModel = (animSrc.modelAnimSouce != nullptr) ? *animSrc.modelAnimSouce : *m_modelToAnimate;
		const ModelAnimation* const animationSourceAnimation = animationSourceModel.animationAt(playback.iAnimation);

		const float evalTime = playback.timeInAnimation;

		for (int iOrigNode = 0; iOrigNode < m_modelToAnimate->numNodes(); ++iOrigNode) {
			// Use the node form the specified Model in the node, if such node doesn't exists, fallback to the originalNode.
			int donorNodeIndex = -1;
			if (animSrc.modelAnimSouce) {
				donorNodeIndex = m_perModel_srcNode_toNode[&animationSourceModel][iOrigNode];
			} else {
				donorNodeIndex = iOrigNode;
			}

			// Find the node that is equvalent to the node in @m_model and evaluate its transform.
			// If no such node was found use the default transformation from @m_model.
			transf3d nodeLocalTransform;
			if (donorNodeIndex >= 0) {
				if (animationSourceAnimation != nullptr) {
					animationSourceAnimation->evaluateForNode(nodeLocalTransform, donorNodeIndex, evalTime);
				} else {
					nodeLocalTransform = animationSourceModel.nodeAt(donorNodeIndex)->staticLocalTransform;
				}
			} else {
				// In no matching node is found, apply the static transform that has no animation.
				nodeLocalTransform = m_modelToAnimate->nodeAt(iOrigNode)->staticLocalTransform;
			}

			// Caution: [EVAL_MESH_NODE_DEFAULT_ZERO]
			// It is assumed that all transforms in evalNode are initialized to zero!
			// Compute the local transform of each node. Once we are done with all nodes
			// we are going to convert these to global transforms.
			outNodeTransforms[iOrigNode] += nodeLocalTransform.toMatrix() * playback.normWeight;
		}
	}

	// Evaluate the node global transform by traversing the node hierarchy using the
	// local transform computed above.
	std::function<void(int, mat4f)> traverseGlobalTransform;
	traverseGlobalTransform = [&](int iNode, const mat4f& parentTransfrom) -> void {
		outNodeTransforms[iNode] = parentTransfrom * outNodeTransforms[iNode];
		for (const int childNodeIndex : m_modelToAnimate->nodeAt(iNode)->childNodes) {
			traverseGlobalTransform(childNodeIndex, outNodeTransforms[iNode]);
		}
	};

	traverseGlobalTransform(m_modelToAnimate->getRootNodeIndex(), mat4f::getIdentity());
}

} // namespace sge
