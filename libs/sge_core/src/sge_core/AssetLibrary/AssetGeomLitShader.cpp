#include "AssetGeomLitShader.h"
#include "sge_core/ICore.h"
#include "sge_utils/io/FileStream.h"
#include "sge_utils/json/json.h"
#include "sge_utils/text/Path.h"
#include "sge_utils/hash/hash_combine.h"

#include "sge_core/materials/MaterialFamilyList.h"

namespace sge {

bool AssetGeomLitShader::loadAssetFromFile(const char* const path)
{
	try {
		mainShaderFile.clear();

		std::string jsonText;
		FileReadStream frs;
		JsonParser jp;
		if (frs.open(path) && jp.parse(&frs)) {
			const JsonValue* jGeomLitRoot = jp.getRoot();
			mainShaderFile = jGeomLitRoot->getMemberOrThrow("shaderMainFile").GetStringOrThrow();
		}
	}
	catch (...) {
		return false;
	}

	bool succeeded = false;
	if (!mainShaderFile.empty()) {
		succeeded = genericShader.createWithCodeFromFile(mainShaderFile.c_str());
	}

	if (succeeded) {
	}

	return succeeded;
}

} // namespace sge
