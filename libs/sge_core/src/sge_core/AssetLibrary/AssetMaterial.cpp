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

bool AssetMaterial::saveAssetToFile(const char* const path) const {
	if (m_material) {
		JsonValueBuffer jvb;
		JsonValue* jMtl = m_material->toJson(jvb, extractFileDir(path, true).c_str());

		JsonWriter jw;
		return jw.WriteInFile(path, jMtl, true);
	}
	return false;
}



} // namespace sge
