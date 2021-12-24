#include "ModelAnimator2.h"
#include "sge_core/AssetLibrary/AssetLibrary.h"
#include "sge_core/ICore.h"
#include "sge_core/model/Model.h"
#include "sge_utils/utils/range_loop.h"
#include <functional>

namespace sge {


void ModelAnimator2::create(Model& modelToBeAnimated) {
	*this = ModelAnimator2();
	this->m_modelToAnimate = &modelToBeAnimated;
}


void ModelAnimator2::trackSetFadeTime(int trackId, float fadingTime) {
	if (m_tracks.count(trackId) == 0) {
		m_tracks[trackId] = AnimationTrack();
	}

	m_tracks[trackId].fadeInOutTime = fadingTime;
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

void ModelAnimator2::trackAddAmimPath(int trackId, const char* modelAssetPath, int animIndexInSrcModel) {
	std::shared_ptr<AssetIface_Model3D> mdlIface = getCore()->getAssetLib()->getLoadedAssetIface<AssetIface_Model3D>(modelAssetPath);
	if (mdlIface) {
		trackAddAmim(trackId, &mdlIface->getModel3D(), animIndexInSrcModel);
	}
}

void ModelAnimator2::trackAddAmimPath(int trackId, const char* modelAssetPath, const char* animNameInModel) {
	std::shared_ptr<AssetIface_Model3D> mdlIface = getCore()->getAssetLib()->getLoadedAssetIface<AssetIface_Model3D>(modelAssetPath);
	if (mdlIface) {
		int animIndex = mdlIface->getModel3D().getAnimationIndexByName(animNameInModel);
		trackAddAmim(trackId, &mdlIface->getModel3D(), animIndex);
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
	playback.trackAnimationIndex = 0; // TODO: randomize this.
	// If there is going to be a fadeout start form 0 weight growing to one, to smoothly transition.
	// otherwise durectly use 100% weight.
	playback.mixingWeight = m_playbacks.empty() ? 1.f : 0.f;

	m_playbacks.emplace_back(playback);
}

void ModelAnimator2::forceTrack(int trackIdToPlay, float animTime) {
	m_playbacks.clear();

	TrackPlayback playback;
	playback.trackId = trackIdToPlay;
	playback.timeInAnimation = animTime;
	playback.trackAnimationIndex = 0;
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

		const AnimationModelSrc& animationSrc = trackInfo.animationSources[playback.trackAnimationIndex];
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
					playback.trackAnimationIndex = 0; // TODO: randomize the animation.
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
			totalWeigth += playback.mixingWeight;
		}

		for (int iPlayback = 0; iPlayback < m_playbacks.size(); ++iPlayback) {
			TrackPlayback& playback = m_playbacks[iPlayback];
			playback.normWeight = m_playbacks[iPlayback].mixingWeight / totalWeigth;
		}
	}
}

int ModelAnimator2::getNumNodes() const {
	return m_modelToAnimate ? m_modelToAnimate->numNodes() : 0;
}

void ModelAnimator2::computeModleNodesTrasnforms(mat4f* outNodeTransforms) {
	// if (outNodeTransforms.size() < m_modelToAnimate->numNodes()) {
	//	sgeAssertFalse("The outNodeTransforms is not big enough!");
	//	return;
	//}

	bool isSingleTrackPlaying = m_playbacks.size() == 1;

	if (!isSingleTrackPlaying) {
		for (int t = 0; t < getNumNodes(); ++t) {
			outNodeTransforms[t] = mat4f::getZero();
		}
	}

	// Evaluates the nodes. They may be effecte by multiple models (stealing animations and blending them)
	for (const TrackPlayback& playback : m_playbacks) {
		const AnimationTrack& track = m_tracks[playback.trackId];
		const AnimationModelSrc& animSrc = track.animationSources[playback.trackAnimationIndex];

		const Model& animationSourceModel = (animSrc.modelAnimSouce != nullptr) ? *animSrc.modelAnimSouce : *m_modelToAnimate;
		const ModelAnimation* const animationSourceAnimation = animationSourceModel.animationAt(animSrc.animIndexInAnimSource);

		const float evalTime = playback.timeInAnimation;

		const int numNodes = m_modelToAnimate->numNodes();
		for (int iOrigNode = 0; iOrigNode < numNodes; ++iOrigNode) {
			// Find the node index in the animation source model.
			int donorNodeIndex = -1;
			if (animSrc.modelAnimSouce) {
				donorNodeIndex = m_perModel_srcNode_toNode[&animationSourceModel][iOrigNode];
			} else {
				donorNodeIndex = iOrigNode;
			}

			// Now evaluate the transform of that node for the animation moment.
			// If no such node was found use the default transformation from @m_model.
			transf3d nodeLocalTransform;
			if (donorNodeIndex >= 0) {
				if (animationSourceAnimation != nullptr) {
					nodeLocalTransform = animationSourceModel.nodeAt(donorNodeIndex)->staticLocalTransform;
					animationSourceAnimation->modifyTransformWithKeyFrames(nodeLocalTransform, donorNodeIndex, evalTime);
				}
			} else {
				// In no matching node is found, apply the static transform that has no animation.
				nodeLocalTransform = m_modelToAnimate->nodeAt(iOrigNode)->staticLocalTransform;
			}

			if (isSingleTrackPlaying) {
				outNodeTransforms[iOrigNode] = nodeLocalTransform.toMatrix();
			} else {
				outNodeTransforms[iOrigNode] += nodeLocalTransform.toMatrix() * playback.normWeight;
			}
		}
	}

	// Evaluate the node global transform by traversing the node hierarchy using the
	// local transform computed above.
	std::function<void(mat4f*, Model*, int, mat4f)> traverseGlobalTransform;
	traverseGlobalTransform = [&traverseGlobalTransform](mat4f* outNodeTransforms, Model* model, int iNode,
	                                                     const mat4f& parentTransfrom) -> void {
		mat4f currNodeGlobalTransform = parentTransfrom * outNodeTransforms[iNode];
		outNodeTransforms[iNode] = currNodeGlobalTransform;
		for (const int childNodeIndex : model->nodeAt(iNode)->childNodes) {
			traverseGlobalTransform(outNodeTransforms, model, childNodeIndex, currNodeGlobalTransform);
		}
	};

	traverseGlobalTransform(outNodeTransforms, m_modelToAnimate, m_modelToAnimate->getRootNodeIndex(), mat4f::getIdentity());
}

} // namespace sge
