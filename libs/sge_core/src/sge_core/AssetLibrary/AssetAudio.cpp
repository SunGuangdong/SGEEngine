#include "AssetAudio.h"

namespace sge {
bool AssetAudio::loadAssetFromFile(const char* const path) {
	m_audioData = std::make_shared<AudioData>();
	m_audioData->createFromFile(path);

	m_status = AssetStatus_Loaded;

	return true;
}
} // namespace sge
