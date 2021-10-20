//#pragma once
//
//#include "sge_engine/Actor.h"
//#include "sge_engine/AssetProperty.h"
//
//namespace sge {
//
////--------------------------------------------------------------------
//// ABackgroundMusic
////--------------------------------------------------------------------
//struct SGE_ENGINE_API ABackgroundMusic : public Actor {
//	ABackgroundMusic();
//	void create() final;
//	void update(const GameUpdateSets& updateSets) final;
//	void onPlayStateChanged(bool const isStartingToPlay) final;
//	AABox3f getBBoxOS() const final;
//
//	void setVolume(float volume);
//
//	AssetProperty m_backgroundMusic;
//	float m_volume = 1.0f;
//
//  private:
//	void setBackgroundMusicInAudioDevice(bool playing);
//	bool m_wasPlaying = false;
//};
//
//
//} // namespace sge
