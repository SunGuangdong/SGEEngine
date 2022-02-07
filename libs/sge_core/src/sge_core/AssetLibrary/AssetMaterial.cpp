#include "AssetMaterial.h"
#include "sge_core/ICore.h"
#include "sge_core/materials/MaterialFamilyList.h"
#include "sge_utils/io/FileStream.h"
#include "sge_utils/json/json.h"
#include "sge_utils/text/Path.h"

namespace sge {

bool AssetMaterial::loadAssetFromFile(const char* const path)
{
	mtl.reset();
	std::string jsonText;
	FileReadStream frs;
	JsonParser jp;
	if (frs.open(path) && jp.parse(&frs)) {
		const JsonValue* jMtlRoot = jp.getRoot();
		std::string mtlDir = extractFileDir(path, true);
		mtl = getCore()->getMaterialLib()->loadMaterialFromJson(jMtlRoot, mtlDir.c_str());
	}

	return mtl != nullptr;
}

bool AssetMaterial::saveAssetToFile(const char* const path) const
{
	if (mtl) {
		JsonValueBuffer jvb;
		JsonValue* jMtl = mtl->toJson(jvb, extractFileDir(path, true).c_str());

		JsonWriter jw;
		return jw.WriteInFile(path, jMtl, true);
	}
	return false;
}



} // namespace sge
