#pragma once

#include "sge_core/typelib/typeLib.h"
#include "sge_log/Log.h"
#include "sge_utils/sge_utils.h"
#include "sge_utils/utils/FileStream.h"
#include "sge_utils/utils/Path.h"
#include "sge_utils/utils/strings.h"
#include "sge_utils/utils/timer.h"

#include "AssetAudio.h"
#include "AssetLibrary.h"
#include "AssetMaterial.h"
#include "AssetModel3D.h"
#include "AssetSpriteAnim.h"
#include "AssetText.h"
#include "AssetTexture2D.h"
#include "IAsset.h"
#include "IAssetInterface.h"

#include <filesystem>

namespace sge {

const char* assetIface_getName(const AssetIfaceType type) {
	switch (type) {
		case assetIface_model3d:
			return "3D Model";
		case assetIface_texture2d:
			return "Texture";
		case assetIface_text:
			return "Text";
		case assetIface_spriteAnim:
			return "Sprite Animation";
		case assetIface_audio:
			return "Audio";
		case assetIface_mtl:
			return "Material";
		default:
			sgeAssertFalse("Not implemented");
			return "NotImplemented";
	}
}

AssetIfaceType assetIface_guessFromExtension(const char* const ext, bool includeExternalExtensions) {
	if (ext == nullptr) {
		return assetIface_unknown;
	}

	if (sge_stricmp(ext, "mdl") == 0) {
		return assetIface_model3d;
	}

	if (includeExternalExtensions && sge_stricmp(ext, "fbx") == 0) {
		return assetIface_model3d;
	}

	if (includeExternalExtensions && sge_stricmp(ext, "dae") == 0) {
		return assetIface_model3d;
	}

	if (includeExternalExtensions && sge_stricmp(ext, "obj") == 0) {
		return assetIface_model3d;
	}

	if (sge_stricmp(ext, "png") == 0 || sge_stricmp(ext, "dds") == 0 || sge_stricmp(ext, "jpg") == 0 || sge_stricmp(ext, "tga") == 0 ||
	    sge_stricmp(ext, "bmp") == 0 || sge_stricmp(ext, "hdr") == 0) {
		return assetIface_texture2d;
	}

	if (sge_stricmp(ext, "txt") == 0) {
		return assetIface_text;
	}

	if (sge_stricmp(ext, "sprite") == 0) {
		return assetIface_spriteAnim;
	}

	if (sge_stricmp(ext, "ogg") == 0 || sge_stricmp(ext, "mp3") == 0 || sge_stricmp(ext, "wav") == 0) {
		return assetIface_audio;
	}

	if (sge_stricmp(ext, "mtl") == 0) {
		return assetIface_mtl;
	}

	return assetIface_unknown;
}

bool AssetLibrary::hasAsset(const std::string& path) const {
	auto finditr = m_allAssets.find(path);
	const bool result = finditr != m_allAssets.end();
	return result;
}

const std::map<std::string, AssetPtr>& AssetLibrary::getAllAssets() const {
	return m_allAssets;
}

void AssetLibrary::scanForAvailableAssets(const char* const path) {
	using namespace std;

	m_gameAssetsDir = absoluteOf(path);
	sgeAssert(m_gameAssetsDir.empty() == false);

	if (filesystem::is_directory(path)) {
		for (const filesystem::directory_entry& entry : filesystem::recursive_directory_iterator(path)) {
			if (entry.status().type() == filesystem::file_type::regular) {
				// Extract the extension of the found file use it for guessing the type of the asset,
				// then mark the asset as existing without loading it.
				const std::string ext = entry.path().extension().u8string();
				const char* extCStr = ext.c_str();

				// the extension() method returns the dot, however assetIface_guessFromExtension
				// needs the extension without the dot.
				if (extCStr != nullptr && extCStr[0] == '.') {
					extCStr++;

					const AssetIfaceType guessedType = assetIface_guessFromExtension(extCStr, false);
					if (guessedType != assetIface_unknown) {
						// markThatAssetExists(entry.path().generic_u8string().c_str(), guessedType);
						getAssetFromFile(entry.path().generic_u8string().c_str(), nullptr, false);
					}
				}
			}
		}
	}
}

AssetPtr AssetLibrary::newAsset(std::string assetPath, AssetIfaceType type) {
	AssetPtr newlyLoadedAsset;

	switch (type) {
		case sge::assetIface_texture2d: {
			auto assetTyped = newAsset<AssetTexture2d>(assetPath.c_str());
			newlyLoadedAsset = assetTyped;
		} break;
		case sge::assetIface_model3d: {
			auto assetTyped = newAsset<AssetModel3D>(assetPath.c_str());
			newlyLoadedAsset = assetTyped;
		} break;
		case sge::assetIface_spriteAnim: {
			auto assetTyped = newAsset<AssetSpriteAnim>(assetPath.c_str());
			newlyLoadedAsset = assetTyped;
		} break;
		case sge::assetIface_audio: {
			auto assetTyped = newAsset<AssetAudio>(assetPath.c_str());
			newlyLoadedAsset = assetTyped;
		} break;
		case sge::assetIface_text: {
			auto assetTyped = newAsset<AssetText>(assetPath.c_str());
			newlyLoadedAsset = assetTyped;
		} break;
		case sge::assetIface_mtl: {
			auto assetTyped = newAsset<AssetMaterial>(assetPath.c_str());
			newlyLoadedAsset = assetTyped;
		} break;
		default:
			sgeAssert(false);
			break;
	}

	return newlyLoadedAsset;
}

std::string AssetLibrary::resloveAssetPathToRelative(const char* pathRaw) const {
	if (pathRaw == nullptr || pathRaw[0] == '\0') {
		return std::string();
	}

	// Now make the pathRaw relative to the current directory, as some assets
	// might refer to this asset relativly, and this would lead us loading the same asset
	// via different path and we don't want that.
	std::error_code pathToAssetRelativeError;
	const std::filesystem::path pathToAssetRelative = std::filesystem::relative(pathRaw, pathToAssetRelativeError);

	// The commented code below, not only makes the path cannonical, but it also makes it absolute, which we don't want.
	// std::error_code pathToAssetCanonicalError;
	// const std::filesystem::path pathToAssetCanonical = std::filesystem::weakly_canonical(pathToAssetRelative, pathToAssetCanonicalError);

	if (pathToAssetRelativeError) {
		sgeAssert(false && "Failed to transform the asset path to relative");
	}

	// canonizePathRespectOS makes makes the slashes UNUX Style.
	std::string pathToAsset = canonizePathRespectOS(pathToAssetRelative.string());
	return std::move(pathToAsset);
}

AssetPtr AssetLibrary::getAssetFromFile(const char* path, const char* localDirectory, bool loadIfMissing) {
	if (path == nullptr || path[0] == '\0') {
		return nullptr;
	}

	const double loadStartTime = Timer::now_seconds();

	std::string pathToAsset;
	if (localDirectory) {
		std::string localPath = std::string(localDirectory) + "/" + path;
		pathToAsset = resloveAssetPathToRelative(localPath.c_str());
	}

	if (pathToAsset.empty()) {
		// If the local path failed, try just the path.
		pathToAsset = resloveAssetPathToRelative(path);
	}

	if (pathToAsset.empty()) {
		return nullptr;
	}

	const AssetIfaceType assetType = assetIface_guessFromExtension(extractFileExtension(path).c_str(), false);
	if (assetType == assetIface_unknown) {
		sgeAssert(false);
		return nullptr;
	}

	// Check if the asset is already loaded, if so just return it.
	auto itrFindAssetByPath = m_allAssets.find(pathToAsset);
	if (itrFindAssetByPath != m_allAssets.end() && isAssetLoaded(itrFindAssetByPath->second)) {
		return itrFindAssetByPath->second;
	}

	if (!loadIfMissing) {
		if (itrFindAssetByPath == m_allAssets.end()) {
			// If the asset has not been created, create one but do not load it.
			// That way it will pop-up in the interface as being available.
			AssetPtr newAssetToMarkExisting = newAsset(pathToAsset.c_str(), assetType);
			m_allAssets[pathToAsset] = newAssetToMarkExisting;
			return newAssetToMarkExisting;
		} else {
			// The asset is already allocated but not loaded.
			// The user doesn't want us to load it, so just return it.
			return itrFindAssetByPath->second;
		}
	}

	AssetPtr assetToModify =
	    itrFindAssetByPath != m_allAssets.end() ? itrFindAssetByPath->second : newAsset(pathToAsset.c_str(), assetType);

	sgeAssert(isAssetSupportingInteface(assetToModify, assetType));
	const bool loadSucceeded = assetToModify->loadAssetFromFile(pathToAsset.c_str());
	assetToModify->m_status = loadSucceeded ? AssetStatus_Loaded : AssetStatus_LoadFailed;
	assetToModify->m_loadAssetFromFileData.lastAcessTime = FileReadStream::getFileModTime(pathToAsset.c_str());

	// Measure the loading time.
	const float loadEndTime = Timer::now_seconds();
	sgeLogInfo("Asset '%s' loaded in %f seconds.\n", pathToAsset.c_str(), loadEndTime - loadStartTime);

	return assetToModify;
}

bool AssetLibrary::reloadAssetModified(AssetPtr& assetToModify) {
	if (!assetToModify) {
		sgeAssert(false);
		return false;
	}

	if (assetToModify->getStatus() != AssetStatus_Loaded && assetToModify->getStatus() != AssetStatus_LoadFailed) {
		return false;
	}

	// Check if the file has changed at all, if not there is no point trying again.
	const sint64 fileNewModTime = FileReadStream::getFileModTime(assetToModify->getPath().c_str());
	if (assetToModify->m_loadAssetFromFileData.lastAcessTime == fileNewModTime) {
		return false;
	}

	const double reloadStartTime = Timer::now_seconds();

	const bool loadSucceeded = assetToModify->loadAssetFromFile(assetToModify->getPath().c_str());
	assetToModify->m_status = loadSucceeded ? AssetStatus_Loaded : AssetStatus_LoadFailed;
	assetToModify->m_loadAssetFromFileData.lastAcessTime = FileReadStream::getFileModTime(assetToModify->getPath().c_str());

	if (!loadSucceeded) {
		sgeAssert(false);
		return false;
	}

	// Measure the loading time.
	const float reloadEndTime = Timer::now_seconds();
	sgeLogInfo("Asset '%s' reloaded in %f seconds.\n", assetToModify->getPath().c_str(), reloadEndTime - reloadStartTime);

	return true;
}

SGE_CORE_API bool isAssetSupportingInteface(const Asset& asset, AssetIfaceType type) {
	switch (type) {
		case sge::assetIface_unknown:
			return true;
			break;
		case sge::assetIface_texture2d:
			return dynamic_cast<const AssetIface_Texture2D*>(&asset) != nullptr;
			break;
		case sge::assetIface_model3d:
			return dynamic_cast<const AssetIface_Model3D*>(&asset) != nullptr;
			break;
		case sge::assetIface_spriteAnim:
			return dynamic_cast<const AssetIface_SpriteAnim*>(&asset) != nullptr;
			break;
		case sge::assetIface_audio:
			return dynamic_cast<const IAssetInterface_Audio*>(&asset) != nullptr;
			break;
		case sge::assetIface_text:
			return dynamic_cast<const IAssetInterface_Text*>(&asset) != nullptr;
			break;
		case sge::assetIface_mtl:
			return dynamic_cast<const AssetIface_Material*>(&asset) != nullptr;
			break;
		default:
			sgeAssert(false && "Not implemented asset interface type");
			break;
	}

	return false;
}

SGE_CORE_API bool isAssetSupportingInteface(const AssetPtr& asset, AssetIfaceType type) {
	return asset && isAssetSupportingInteface(*asset.get(), type);
}

SGE_CORE_API bool isAssetLoaded(Asset& asset, AssetIfaceType type) {
	if (isAssetLoaded(asset) == false) {
		return false;
	}
	switch (type) {
		case sge::assetIface_unknown:
			return true;
			break;
		case sge::assetIface_texture2d:
			return dynamic_cast<AssetIface_Texture2D*>(&asset) != nullptr;
			break;
		case sge::assetIface_model3d:
			return dynamic_cast<AssetIface_Model3D*>(&asset) != nullptr;
			break;
		case sge::assetIface_spriteAnim:
			return dynamic_cast<AssetIface_SpriteAnim*>(&asset) != nullptr;
			break;
		case sge::assetIface_audio:
			return dynamic_cast<IAssetInterface_Audio*>(&asset) != nullptr;
			break;
		case sge::assetIface_text:
			return dynamic_cast<IAssetInterface_Text*>(&asset) != nullptr;
			break;
		case sge::assetIface_mtl:
			return dynamic_cast<AssetIface_Material*>(&asset) != nullptr;
			break;
		default:
			break;
	}

	sgeAssert(false);
	return false;
}

/// Returns true if the specified asset is loaded and it supports the specified interface.
bool isAssetLoaded(const AssetPtr& asset, AssetIfaceType type) {
	if (isAssetLoaded(asset) == false) {
		return false;
	}

	return isAssetLoaded(*asset.get(), type);
}

// clang-format off
ReflAddTypeId(AssetPtr,                                             10'11'21'0001);
ReflAddTypeId(std::vector<AssetPtr>,                                10'11'21'0002);
ReflAddTypeId(std::shared_ptr<AssetIface_Texture2D>,                10'11'21'0003);
ReflAddTypeId(std::vector<std::shared_ptr<AssetIface_Texture2D>>,   10'11'21'0004);
ReflAddTypeId(std::shared_ptr<AssetIface_Model3D>,                  10'11'21'0005);
ReflAddTypeId(std::vector<std::shared_ptr<AssetIface_Model3D>>,     10'11'21'0006);
ReflAddTypeId(std::shared_ptr<AssetIface_SpriteAnim>,               10'11'21'0007);
ReflAddTypeId(std::vector<std::shared_ptr<AssetIface_SpriteAnim>>,  10'11'21'0008);
ReflAddTypeId(std::shared_ptr<AssetIface_Material>,                 10'11'21'0009);
ReflAddTypeId(std::vector<std::shared_ptr<AssetIface_Material>>,    10'11'21'0010);
ReflBlock() {
	ReflAddType(AssetPtr);
	ReflAddType(std::vector<AssetPtr>);
	ReflAddType(std::shared_ptr<AssetIface_Texture2D>);
	ReflAddType(std::vector<std::shared_ptr<AssetIface_Texture2D>>);
	ReflAddType(std::shared_ptr<AssetIface_Model3D>);
	ReflAddType(std::vector<std::shared_ptr<AssetIface_Model3D>>);
	ReflAddType(std::shared_ptr<AssetIface_SpriteAnim>);
	ReflAddType(std::vector<std::shared_ptr<AssetIface_SpriteAnim>>);
	ReflAddType(std::shared_ptr<AssetIface_Material>);
	ReflAddType(std::vector<std::shared_ptr<AssetIface_Material>>);
}
// clang-format on

} // namespace sge
