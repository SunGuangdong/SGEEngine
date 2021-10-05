#include "ABackgroundMusic.h"
#include "sge_core/ICore.h"
#include "sge_engine/GameWorld.h"

namespace sge {

//--------------------------------------------------------------------
// ABackgroundMusic
//--------------------------------------------------------------------
// clang-format off
RelfAddTypeId(ABackgroundMusic, 21'04'28'0001);
ReflBlock() {
	ReflAddActor(ABackgroundMusic)
		ReflMember(ABackgroundMusic, m_backgroundMusic)
		ReflMember(ABackgroundMusic, m_volume).uiRange(0.0f, 1.0f, 0.01f)
	;
}
// clang-format on

AABox3f ABackgroundMusic::getBBoxOS() const {
	return AABox3f();
}

void ABackgroundMusic::setBackgroundMusicInAudioDevice(bool playing) {
	AudioAsset* const audioAsset = m_backgroundMusic.getAssetAudio();
	AudioTrack* const audioTrack = audioAsset && *audioAsset ? audioAsset->get() : nullptr;

	if (!audioTrack) {
		return;
	}

	if (playing) {
		audioTrack->trackVolume = m_volume;
		getCore()->getAudioDevice()->setBackgroundMusic(*audioAsset);
	} else {
		getCore()->getAudioDevice()->setBackgroundMusic(nullptr);
	}
}

ABackgroundMusic::ABackgroundMusic()
    : m_backgroundMusic(AssetType::Audio) {
}

void ABackgroundMusic::create() {
}

void ABackgroundMusic::update(const GameUpdateSets& updateSets) {
	Actor::update(updateSets);
	const bool assetChanged = m_backgroundMusic.update();
	AudioAsset* const audioAsset = m_backgroundMusic.getAssetAudio();
	AudioTrack* const audioTrack = audioAsset && *audioAsset ? audioAsset->get() : nullptr;

	if (assetChanged && audioTrack != nullptr) {
		audioTrack->decoder.seekToBegining();
	}

	if (m_wasPlaying && updateSets.isGamePaused()) {
		setBackgroundMusicInAudioDevice(false);
	} else if (!m_wasPlaying && updateSets.isPlaying()) {
		setBackgroundMusicInAudioDevice(true);
	} else if (updateSets.isPlaying() && assetChanged) {
		setBackgroundMusicInAudioDevice(true);
	} else if (audioTrack != nullptr && (*audioAsset)->trackVolume != m_volume) {
		setBackgroundMusicInAudioDevice(true);
	}
	m_wasPlaying = updateSets.isPlaying();
}

void ABackgroundMusic::onPlayStateChanged(bool const isStartingToPlay) {
	Actor::onPlayStateChanged(isStartingToPlay);
	if (!isStartingToPlay) {
		setBackgroundMusicInAudioDevice(false);
	}

	AudioAsset* const audioAsset = m_backgroundMusic.getAssetAudio();
	AudioTrack* const audioTrack = audioAsset && *audioAsset ? audioAsset->get() : nullptr;

	if (audioTrack) {
		audioTrack->decoder.seekToBegining();
	}
}

} // namespace sge
