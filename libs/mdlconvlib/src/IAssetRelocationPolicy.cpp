#include "IAssetRelocationPolicy.h"

#include "sge_utils/text/Path.h"

namespace sge {

std::string RelativeAssetRelocationPolicy::whatWillBeTheAssetNameOf(const std::string& askerDir, const char* path) {
	std::string assetName = canonizePathRespectOS(askerDir + path);
	return assetName;
}

#if 0
std::string SeparateTypeAssetRelocationPolicy::whatWillBeTheAssetNameOf(const std::string& askerDir, const char* path)
{
	const std::string filename = extractFileNameWithExt(path);
	const std::string ext = extractFileExtension(path);

	std::string result;

	if(ext == "png" || ext == "bmp" || ext == "dds" || ext == "rga" || ext == "jpg"){
		return "textures/" + filename;
	}
	else if(ext == "dae" || ext == "fbx" || ext =="obj"){
		result = "models/" + replaceExtension(filename.c_str(), "mdl");
	}

	sgeAssert(false);
	return "unknown/" + filename;
}
#endif

} // namespace sge
