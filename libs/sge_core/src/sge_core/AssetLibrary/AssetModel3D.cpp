#include "AssetModel3D.h"
#include "sge_core/ICore.h"
#include "sge_core/model/ModelReader.h"
#include "sge_log/Log.h"
#include "sge_utils/io/FileStream.h"
#include "sge_utils/text/Path.h"

namespace sge {
bool AssetModel3D::loadAssetFromFile(const char* const path)
{
	m_status = AssetStatus_LoadFailed;

	FileReadStream frs(path);

	if (frs.isOpened() == false) {
		sgeLogError("Unable to find model asset: '%s'!\n", path);
		return false;
	}

	// Reset the option to a valid value.
	m_modelOpt = Model();

	ModelLoadSettings loadSettings;
	loadSettings.assetDir = extractFileDir(path, true);

	ModelReader modelReader;
	const bool succeeded = modelReader.loadModel(loadSettings, &frs, m_modelOpt.get());

	if (!succeeded) {
		sgeLogError("Unable to load model asset: '%s'!\n", path);
		return false;
	}

	m_modelOpt->prepareForRendering(*getCore()->getDevice(), m_ownerAssetLib);

	m_staticEval.initialize(m_modelOpt.getPtr());
	m_staticEval.evaluateStatic();

	if (succeeded) {
		m_status = AssetStatus_Loaded;
	}

	return m_status == AssetStatus_Loaded;
}
} // namespace sge
