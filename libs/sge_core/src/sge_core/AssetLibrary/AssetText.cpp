#include "AssetText.h"
#include "sge_utils/io/FileStream.h"

namespace sge {
bool AssetText::loadAssetFromFile(const char* const path) {
	m_status = AssetStatus_LoadFailed;

	m_text.clear();
	if (!FileReadStream::readTextFile(path, m_text)) {
		return false;
	}

	m_status = AssetStatus_Loaded;

	return true;
}
} // namespace sge
