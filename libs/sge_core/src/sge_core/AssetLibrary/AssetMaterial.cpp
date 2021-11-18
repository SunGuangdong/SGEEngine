#include "AssetMaterial.h"
#include "sge_core/ICore.h"
#include "sge_core/materials/MaterialFamilyList.h"
#include "sge_utils/utils/FileStream.h"
#include "sge_utils/utils/Path.h"
#include "sge_utils/utils/json.h"

namespace sge {

bool AssetMaterial::loadAssetFromFile(const char* const path) {
	m_material = nullptr;
	std::string jsonText;
	FileReadStream frs;
	JsonParser jp;
	if (frs.open(path) && jp.parse(&frs)) {
		const JsonValue* jMtlRoot = jp.getRoot();
		std::string mtlDir = extractFileDir(path, true);
		m_material = getCore()->getMaterialLib()->loadMaterialFromJson(jMtlRoot, mtlDir.c_str());
	}

	return m_material != nullptr;
}



} // namespace sge
