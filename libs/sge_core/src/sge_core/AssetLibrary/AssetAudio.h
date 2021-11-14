#pragma once

#include "IAsset.h"
#include "IAssetInterface.h"
#include "sge_audio/AudioDevice.h"


namespace sge {

struct SGE_CORE_API IAssetInterface_Audio : public IAssetInterface {
	virtual AudioDataPtr getAudioData() = 0;
};

struct SGE_CORE_API AssetAudio : public Asset, public IAssetInterface_Audio {
	AssetAudio(std::string assetPath, AssetLibrary& ownerAssetLib)
	    : Asset(assetPath, ownerAssetLib) {
	}

	virtual AudioDataPtr getAudioData() override {
		return m_audioData;
	}

	bool loadAssetFromFile(const char* const path) override;

  public:
	AudioDataPtr m_audioData;
};

} // namespace sge
