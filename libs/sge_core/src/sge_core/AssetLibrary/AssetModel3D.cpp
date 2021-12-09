#include "AssetModel3D.h"
#include "sge_core/ICore.h"
#include "sge_core/model/ModelReader.h"
#include "sge_log/Log.h"
#include "sge_utils/utils/FileStream.h"
#include "sge_utils/utils/Path.h"

namespace sge {
bool AssetModel3D::loadAssetFromFile(const char* const path) {
	m_status = AssetStatus_LoadFailed;

	FileReadStream frs(path);

	if (frs.isOpened() == false) {
		sgeLogError("Unable to find model asset: '%s'!\n", path);
		// sgeAssert(false);
		return false;
	}

	ModelLoadSettings loadSettings;
	loadSettings.assetDir = extractFileDir(path, true);

	ModelReader modelReader;
	const bool succeeded = modelReader.loadModel(loadSettings, &frs, m_model);

	if (!succeeded) {
		sgeLogError("Unable to load model asset: '%s'!\n", path);
		// sgeAssert(false);
		return false;
	}

	m_model.prepareForRendering(*getCore()->getDevice(), m_ownerAssetLib);

	m_staticEval.initialize(&m_ownerAssetLib, &m_model);
	m_staticEval.evaluateStatic();

	if (succeeded) {
		m_status = AssetStatus_Loaded;
	}

	return m_status == AssetStatus_Loaded;
}
} // namespace sge
